using System.Collections.Immutable;

namespace ShardScript.Scripting;

/// <summary>
/// Immutable options that control how a ShardScript session is created and executed.
/// </summary>
public sealed class ShardScriptOptions
{
    public static ShardScriptOptions Default { get; } = new ShardScriptOptions();

    public bool LoadStandardLibraries { get; }
    public bool EntryPointEnabled { get; }
    public string? DefaultNamespace { get; }
    public ImmutableArray<string> LibraryPaths { get; }
    public ImmutableArray<string> SourcePaths { get; }

    private ShardScriptOptions(
        bool loadStandardLibraries = false,
        bool entryPointEnabled = true,
        string? defaultNamespace = null,
        ImmutableArray<string>? libraryPaths = null,
        ImmutableArray<string>? sourcePaths = null)
    {
        LoadStandardLibraries = loadStandardLibraries;
        EntryPointEnabled = entryPointEnabled;
        DefaultNamespace = defaultNamespace;
        LibraryPaths = libraryPaths ?? ImmutableArray<string>.Empty;
        SourcePaths = sourcePaths ?? ImmutableArray<string>.Empty;
    }

    public ShardScriptOptions WithStandardLibraries()
        => new ShardScriptOptions(true, EntryPointEnabled, DefaultNamespace, LibraryPaths, SourcePaths);

    public ShardScriptOptions WithNamespace(string ns)
    {
        if (ns == null)
            throw new ArgumentNullException(nameof(ns));

        return new(LoadStandardLibraries, EntryPointEnabled, ns, LibraryPaths, SourcePaths);
    }

    public ShardScriptOptions WithLibrary(string path)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        return new(LoadStandardLibraries, EntryPointEnabled, DefaultNamespace, LibraryPaths.Add(path), SourcePaths);
    }

    public ShardScriptOptions WithSourceFile(string path)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        return new(LoadStandardLibraries, EntryPointEnabled, DefaultNamespace, LibraryPaths, SourcePaths.Add(path));
    }

    public ShardScriptOptions WithEntryPoint(bool enabled)
        => new(LoadStandardLibraries, enabled, DefaultNamespace, LibraryPaths, SourcePaths);
}
