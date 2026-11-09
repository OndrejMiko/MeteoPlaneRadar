using System.Security.Cryptography;

namespace MeteoChat.Core;

public interface IProtector
{
    byte[] Protect(byte[] bytes);
    byte[] Unprotect(byte[] bytes);
}
public sealed class StateStore(string folder, IProtector protector)
{
    public BridgeState Read()
    {
        var file = Path.Combine(folder, "state.dat");
        if (!File.Exists(file)) return new BridgeState();
        if (new FileInfo(file).Length > NativeHttp.MaxResponse) throw new BridgeException("Uložené nastavenia sú príliš veľké.");
        var plain = protector.Unprotect(File.ReadAllBytes(file));
        try
        {
            var state = Json.Read<BridgeState>(plain); state.Pair?.Validate();
            foreach (var space in state.Spaces) space.Validate();
            return state;
        }
        finally { CryptographicOperations.ZeroMemory(plain); }
    }
    public void Write(BridgeState state)
    {
        Directory.CreateDirectory(folder);
        var plain = Json.Write(state); byte[] encrypted;
        if (plain.Length > NativeHttp.MaxResponse - 1024) { CryptographicOperations.ZeroMemory(plain); throw new BridgeException("Uložené nastavenia sú príliš veľké."); }
        try { encrypted = protector.Protect(plain); }
        finally { CryptographicOperations.ZeroMemory(plain); }
        var temporary = Path.Combine(folder, ".state-" + Guid.NewGuid().ToString("N") + ".tmp");
        try
        {
            using (var stream = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough))
            { stream.Write(encrypted); stream.Flush(true); }
            File.Move(temporary, Path.Combine(folder, "state.dat"), true);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
}
