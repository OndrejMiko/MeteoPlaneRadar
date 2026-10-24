"""Offline integration tests: real HTTP delivery, retry checkpoints, OAuth refresh."""
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
import unittest
import urllib.error
from http.server import BaseHTTPRequestHandler, HTTPServer
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('bridge', Path(__file__).resolve().parents[1] / 'tools/google-chat-bridge/bridge.py')
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.folder = Path(self.tmp.name)
        bridge.save(self.folder / 'config.json', dict(client={'client_id': 'test'}, pair={'device': 'http://127.0.0.1/api/notifications', 'key': 'a'*64}, spaces=[{'name': 'spaces/one'}], self='own'))
        bridge.save(self.folder / 'tokens.json', {'access_token': 'test', 'refresh_token': 'refresh', 'expires_at': 99999999999})
        bridge.save(self.folder / 'state.json', {'spaces': {'spaces/one': {'cursor': '2026-10-24T00:00:00Z'}}})
        self.worker = bridge.Bridge(self.folder)
        self.space = {'name': 'spaces/one', 'displayName': 'Work'}

    def tearDown(self):
        self.tmp.cleanup()

    def message(self, i, sender='users/other'):
        return dict(name=f'spaces/one/messages/{i}', sender={'name': sender}, text='Hello', createTime='2026-10-24T12:00:00Z')

    def test_retry_same_timestamp_restart_and_page_boundary(self):
        page = {'messages': [self.message(1), self.message(2)], 'nextPageToken': 'page2'}
        delivered = []
        def device(payload):
            delivered.append(payload['id'])
            if len(delivered) == 2:
                error = urllib.error.HTTPError('http://device', 503, 'Full', {}, None)
                error.close()
                raise error
        self.worker.google = lambda *args: page
        self.worker.device = device
        with self.assertRaises(urllib.error.HTTPError):
            self.worker.poll(self.space)
        self.assertEqual(bridge.load(self.folder / 'state.json')['spaces']['spaces/one']['cursor'], '2026-10-24T00:00:00Z')
        # Restart: only unacknowledged message is retried, even at equal timestamp.
        self.worker = bridge.Bridge(self.folder)
        self.worker.google = lambda *args: page
        self.worker.device = lambda payload: delivered.append(payload['id'])
        self.worker.poll(self.space)
        self.assertEqual(delivered[1], delivered[2])
        self.assertNotEqual(delivered[0], delivered[2])
        params_seen = []
        def last_page(endpoint, params):
            params_seen.append(params)
            return {'messages': [self.message(3), self.message(4, 'users/own')]}
        self.worker.google = last_page
        self.worker.poll(self.space)
        self.assertEqual(params_seen[0]['pageToken'], 'page2')
        self.assertEqual(params_seen[0]['filter'], 'createTime > "2026-10-24T00:00:00Z"')
        state = bridge.load(self.folder / 'state.json')['spaces']['spaces/one']
        self.assertEqual(state['cursor'], '2026-10-24T12:00:00Z')
        self.assertNotIn('pageToken', state)
        self.assertEqual(state['accepted'], [])
        self.assertEqual(len(delivered), 4)  # includes failed attempt, excludes own

    def test_http_wire_auth_utf8_bound_and_no_redirect(self):
        records = []
        class Device(BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(302)
                self.send_header('Location', '/must-not-follow')
                self.end_headers()
            def do_POST(self):
                records.append((self.path, self.headers.get('Authorization'), json.loads(self.rfile.read(int(self.headers['Content-Length'])))))
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b'{"ok":true}')
            def log_message(self, *args):
                pass
        with HTTPServer(('127.0.0.1', 0), Device) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            self.worker.config['pair']['device'] = f'http://127.0.0.1:{server.server_port}/api/notifications'
            msg = self.message(1)
            msg['text'] = 'Ž🙂'*1000
            payload = bridge.make_payload(msg, self.space, {'users/other': 'Matúš'})
            self.worker.device(payload)
            self.assertEqual(records[0][0], '/api/notifications')
            self.assertEqual(records[0][1], 'Bearer '+'a'*64)
            self.assertLessEqual(len(records[0][2]['text'].encode()), 1024)
            self.assertEqual(records[0][2]['sender'], 'Matúš')
            with self.assertRaises(urllib.error.HTTPError) as error:
                self.worker.device()
            self.assertEqual(error.exception.code, 302)
            server.shutdown()
            thread.join()

    def test_oauth_loopback_state_pkce(self):
        import base64
        import hashlib
        import urllib.parse
        import urllib.request
        captured = {}
        threads = []
        def open_browser(url):
            query = urllib.parse.parse_qs(urllib.parse.urlsplit(url).query)
            captured.update(query)
            def callbacks():
                target = query['redirect_uri'][0]
                try:
                    urllib.request.urlopen(target+'?state=wrong&code=stolen', timeout=2)
                except urllib.error.HTTPError as error:
                    self.assertEqual(error.code, 400)
                    error.close()
                good = target+'?'+urllib.parse.urlencode({'state':query['state'][0], 'code':'valid'})
                with urllib.request.urlopen(good, timeout=2) as response:
                    self.assertEqual(response.status, 200)
            thread = threading.Thread(target=callbacks)
            threads.append(thread)
            thread.start()
        def token_request(url, data, form):
            self.assertEqual(url, 'https://oauth2.googleapis.com/token')
            self.assertEqual(data['code'], 'valid')
            self.assertEqual(captured['code_challenge_method'], ['S256'])
            challenge = base64.urlsafe_b64encode(hashlib.sha256(data['code_verifier'].encode()).digest()).rstrip(b'=').decode()
            self.assertEqual(captured['code_challenge'], [challenge])
            return {'access_token':'new', 'refresh_token':'refresh', 'expires_in':3600}
        with patch.object(bridge.webbrowser, 'open', open_browser), patch.object(bridge, 'request', token_request), patch('builtins.print'):
            token = bridge.oauth({'client_id':'test', 'client_secret':'secret'})
        for thread in threads:
            thread.join()
        self.assertEqual(token['refresh_token'], 'refresh')

    def test_refresh_and_secrets_permissions(self):
        self.worker.token['expires_at'] = 0
        with patch.object(bridge, 'request', side_effect=[{'access_token':'new','expires_in':3600}, {}]) as req:
            self.worker.google('spaces')
        self.assertEqual(req.call_args_list[0].args[0], 'https://oauth2.googleapis.com/token')
        self.assertEqual(req.call_args_list[1].kwargs['headers']['Authorization'], 'Bearer new')
        token = bridge.load(self.folder / 'tokens.json')
        self.assertEqual(token['refresh_token'], 'refresh')
        self.assertEqual((self.folder / 'tokens.json').stat().st_mode & 0o777, 0o600)

    def test_attachment_empty_and_pair_validation(self):
        msg = self.message(1)
        msg['text'] = ''
        self.assertIsNone(bridge.make_payload(msg, self.space, {}))
        msg['attachment'] = [{'contentName': 'test.pdf'}]
        self.assertIn('test.pdf', bridge.make_payload(msg, self.space, {})['text'])
        msg['text'] = '\\'*1024
        payload = bridge.make_payload(msg, self.space, {})
        self.assertLessEqual(len(json.dumps(payload, ensure_ascii=False).encode()), 2048)
        for url in ['http://user:pw@host/api/notifications', 'ftp://host/api/notifications', 'http://host/wrong', 'http://host/api/notifications?token=bad']:
            with self.assertRaises(ValueError):
                bridge.validate_pair({'device': url, 'key':'a'*64})


if __name__ == '__main__':
    unittest.main()
