#!/usr/bin/env python3
"""Google Chat -> MeteoPlaneRadar. Python 3.9+, standard library only.

OAuth and Google tokens stay on this computer. The ESP only accepts bounded
messages authenticated with a separate revocable pairing key. No public port.
"""
import argparse
import base64
import datetime as dt
import hashlib
import http.server
import json
import os
from pathlib import Path
import re
import secrets
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import webbrowser

SCOPES = 'openid https://www.googleapis.com/auth/chat.spaces.readonly https://www.googleapis.com/auth/chat.messages.readonly'
API = 'https://chat.googleapis.com/v1/'
MAX_RESPONSE = 2 * 1024 * 1024


class SetupError(ValueError):
    """An actionable message containing no credentials or API response bodies."""


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        # Never forward device pairing or Google bearer keys to another host.
        return None


def request(url, data=None, headers=None, form=False):
    headers = dict(headers or {})
    if data is not None:
        body = (urllib.parse.urlencode(data) if form else json.dumps(data, ensure_ascii=False)).encode()
        headers['Content-Type'] = 'application/x-www-form-urlencoded' if form else 'application/json'
    else:
        body = None
    req = urllib.request.Request(url, body, headers)
    try:
        with urllib.request.build_opener(NoRedirect()).open(req, timeout=20) as response:
            raw = response.read(MAX_RESPONSE + 1)
    except urllib.error.HTTPError as error:
        error.close()
        raise
    if len(raw) > MAX_RESPONSE:
        raise SetupError('API response exceeds 2 MiB')
    return json.loads(raw)


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    tmp = path.with_suffix('.tmp')
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        os.chmod(tmp, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(value, stream)
        os.replace(tmp, path)
        os.chmod(path, 0o600)
    finally:
        if tmp.exists():
            tmp.unlink()


def load(path):
    return json.loads(Path(path).read_text())


def validate_pair(pair):
    parsed = urllib.parse.urlsplit(pair.get('device', ''))
    if (parsed.scheme not in ('http', 'https') or not parsed.hostname or
            parsed.username or parsed.password or parsed.path != '/api/notifications' or
            parsed.query or parsed.fragment or not re.fullmatch('[0-9a-fA-F]{64}', pair.get('key', ''))):
        raise SetupError('Invalid device pairing file; download it again from Notifications')
    return pair


def oauth(client):
    """Desktop loopback flow, PKCE + state. Callback binds loopback only."""
    state = secrets.token_urlsafe(32)
    verifier = secrets.token_urlsafe(64)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b'=').decode()
    result = {}

    class Callback(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            parsed = urllib.parse.urlsplit(self.path)
            query = urllib.parse.parse_qs(parsed.query)
            valid = parsed.path == '/callback' and secrets.compare_digest(query.get('state', [''])[0], state)
            if not valid:
                self.send_error(400)
                return
            result.update(code=query.get('code', [''])[0], error=query.get('error', [''])[0])
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain; charset=utf-8')
            self.end_headers()
            self.wfile.write(b'Return to the terminal. You can close this tab.')

        def log_message(self, *args):
            pass  # Never log callback codes.

    with http.server.HTTPServer(('127.0.0.1', 0), Callback) as server:
        server.timeout = 1
        redirect = f'http://127.0.0.1:{server.server_port}/callback'
        params = dict(client_id=client['client_id'], redirect_uri=redirect,
                      response_type='code', scope=SCOPES, state=state,
                      code_challenge=challenge, code_challenge_method='S256',
                      access_type='offline', prompt='consent')
        url = 'https://accounts.google.com/o/oauth2/v2/auth?' + urllib.parse.urlencode(params)
        print('Sign in in the browser on this computer. If it did not open, use:\n' + url)
        webbrowser.open(url)
        deadline = time.monotonic() + 300
        while not result and time.monotonic() < deadline:
            server.handle_request()
    if not result.get('code') or result.get('error'):
        raise SetupError('Google sign-in cancelled or timed out')
    token = request('https://oauth2.googleapis.com/token', dict(
        client_id=client['client_id'], client_secret=client.get('client_secret', ''),
        code=result['code'], code_verifier=verifier, redirect_uri=redirect,
        grant_type='authorization_code'), form=True)
    if not token.get('refresh_token'):
        raise SetupError('Google did not issue an offline refresh token; revoke access and run setup again')
    token['expires_at'] = time.time() + token.get('expires_in', 3600)
    return token


class Bridge:
    def __init__(self, folder):
        self.folder = Path(folder)
        self.config = load(self.folder / 'config.json')
        validate_pair(self.config['pair'])
        self.token = load(self.folder / 'tokens.json')
        self.state = load(self.folder / 'state.json')

    def persist(self):
        save(self.folder / 'state.json', self.state)

    def google(self, endpoint, params=None):
        if time.time() >= self.token.get('expires_at', 0) - 60:
            refreshed = request('https://oauth2.googleapis.com/token', dict(
                client_id=self.config['client']['client_id'],
                client_secret=self.config['client'].get('client_secret', ''),
                refresh_token=self.token['refresh_token'], grant_type='refresh_token'), form=True)
            self.token.update(refreshed)
            self.token['expires_at'] = time.time() + refreshed.get('expires_in', 3600)
            save(self.folder / 'tokens.json', self.token)
        url = API + endpoint
        if params:
            url += '?' + urllib.parse.urlencode(params)
        return request(url, headers={'Authorization': 'Bearer ' + self.token['access_token']})

    def device(self, message=None):
        pair = self.config['pair']
        return request(pair['device'] + ('/ping' if message is None else ''), message,
                       headers={'Authorization': 'Bearer ' + pair['key']})

    def poll(self, space):
        """Checkpoint a whole Google page, retaining acknowledgements across restarts.

        Never advance the query lower bound mid-page: equal createTime timestamps
        must survive retries and page boundaries. One bounded page per space/tick.
        """
        name = space['name']
        checkpoint = self.state['spaces'][name]
        params = dict(pageSize=100, orderBy='createTime ASC',
                      filter=f'createTime > "{checkpoint["cursor"]}"')
        if checkpoint.get('pageToken'):
            params['pageToken'] = checkpoint['pageToken']
        page = self.google(name + '/messages', params)
        accepted = checkpoint.setdefault('accepted', [])
        for message in page.get('messages', []):
            msg_id = message['name']
            if msg_id in accepted:
                continue
            sender = message.get('sender', {})
            own = sender.get('name') == 'users/' + self.config.get('self', '')
            if not own and not message.get('deleteTime'):
                payload = make_payload(message, space, self.config.get('aliases', {}))
                if payload:
                    self.device(payload)  # 503/409: no checkpoint; retry later.
            accepted.append(msg_id)
            checkpoint['latest'] = message['createTime']
            self.persist()
        if page.get('nextPageToken'):
            checkpoint['pageToken'] = page['nextPageToken']
        else:
            checkpoint['cursor'] = checkpoint.get('latest', checkpoint['cursor'])
            checkpoint.pop('pageToken', None)
            checkpoint.pop('latest', None)
        checkpoint['accepted'] = []
        self.persist()


def bounded(text, size):
    raw = str(text).encode('utf-8')
    if len(raw) <= size:
        return str(text)
    return raw[:size-3].decode('utf-8', errors='ignore') + '...'


def make_payload(message, space, aliases):
    text = message.get('text', '').strip()
    if not text and message.get('attachment'):
        text = '[Attachment] ' + ', '.join(a.get('contentName', 'file') for a in message['attachment'])
    if not text:
        return None
    sender = message.get('sender', {})
    name = sender.get('name', '')
    payload = dict(service='google-chat', id='chat-' + hashlib.sha256(message['name'].encode()).hexdigest(),
                sender=bounded(aliases.get(name) or sender.get('displayName') or name or 'Google Chat', 127),
                room=bounded(space.get('displayName') or space['name'], 127), text=bounded(text, 1024))
    # JSON escaping can expand backslashes/control characters beyond raw UTF-8 size.
    while len(json.dumps(payload, ensure_ascii=False).encode()) > 2048:
        payload['text'] = bounded(payload['text'], max(3, len(payload['text'].encode()) - 128))
    return payload


def setup(args):
    folder = Path(args.state)
    pair = validate_pair(load(args.pair))
    client = load(args.client).get('installed')
    if not client or not client.get('client_id'):
        raise SetupError('Download an OAuth client JSON of type Desktop app')
    token = oauth(client)
    # Persist credentials before discovery so re-auth is not required on transient failures.
    config = dict(client={k: client[k] for k in ('client_id', 'client_secret') if k in client}, pair=pair, spaces=[])
    save(folder / 'config.json', config)
    save(folder / 'tokens.json', token)
    save(folder / 'state.json', {'spaces': {}})
    bridge = Bridge(folder)
    info = request('https://openidconnect.googleapis.com/v1/userinfo',
                   headers={'Authorization': 'Bearer ' + token['access_token']})
    config['self'] = info['sub']
    spaces, page = [], ''
    while True:
        response = bridge.google('spaces', dict(pageSize=100, **({'pageToken': page} if page else {})))
        spaces.extend(response.get('spaces', []))
        page = response.get('nextPageToken')
        if not page:
            break
    if not spaces:
        raise SetupError('No Chat spaces found. Check Workspace access, membership and Chat API configuration')
    for i, space in enumerate(spaces, 1):
        print(f'{i}. {space.get("displayName", space["name"])} ({space["name"]})')
    selected = input('Space numbers separated by commas (e.g. 1,3): ').strip()
    indices = sorted(set(int(value.strip()) for value in selected.split(',')))
    if not indices or any(i < 1 or i > len(spaces) for i in indices):
        raise SetupError('Invalid selection')
    config['spaces'] = [{k: spaces[i-1][k] for k in ('name', 'displayName') if k in spaces[i-1]} for i in indices]
    # Start now: do not flood the display with the entire Chat history.
    start = dt.datetime.now(dt.timezone.utc).isoformat(timespec='microseconds').replace('+00:00', 'Z')
    save(folder / 'config.json', config)
    save(folder / 'state.json', {'spaces': {s['name']: {'cursor': start} for s in config['spaces']}})
    bridge = Bridge(folder)
    bridge.device()
    print('Paired. Start the run command. Only new messages will be forwarded.')


def run(args):
    bridge = Bridge(args.state)
    if not bridge.config['spaces']:
        raise SetupError('Setup was not completed; run setup again')
    interval, failures = max(5, min(args.interval, 300)), 0
    print('Bridge running. Ctrl+C stops it. Message bodies and credentials are never logged.')
    while True:
        try:
            ping = bridge.device()
            if not ping.get('enabled'):
                print('Notifications disabled on the display. Waiting.')
            else:
                for space in bridge.config['spaces']:
                    bridge.poll(space)
            failures = 0
        except urllib.error.HTTPError as error:
            # Never print URL/body: both can contain private information.
            print(f'HTTP {error.code}. Retry pending; check device pairing/queue or Google permissions.')
            failures = min(failures+1, 5)
        except (OSError, ValueError, KeyError):
            print('Connection or response error. Pending delivery retained; retrying.')
            failures = min(failures+1, 5)
        time.sleep(min(300, interval * 2**failures))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--state', default='.chat-bridge', help='private local state directory')
    sub = parser.add_subparsers(dest='command', required=True)
    init = sub.add_parser('setup')
    init.add_argument('--client', required=True)
    init.add_argument('--pair', required=True)
    init.set_defaults(action=setup)
    worker = sub.add_parser('run')
    worker.add_argument('--interval', type=int, default=15)
    worker.set_defaults(action=run)
    args = parser.parse_args()
    try:
        args.action(args)
    except KeyboardInterrupt:
        print('\nStopped. Pending delivery retained.')
    except urllib.error.HTTPError as error:
        sys.exit(f'HTTP {error.code}: check Google API/OAuth permissions or display pairing. No credentials logged.')
    except SetupError as error:
        sys.exit(str(error))
    except (OSError, ValueError, KeyError) as error:
        sys.exit(f'Setup/configuration error ({type(error).__name__}); check the local files and guide.')


if __name__ == '__main__':
    main()
