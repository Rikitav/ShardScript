using System.Collections.Immutable;

namespace ShardScript.NET.Scripting;

/// <summary>
/// Immutable options that control how a ShardScript session is created and executed.
/// </summary>
public sealed class ScriptOptions
{
    public static ScriptOptions Default { get; } = new ScriptOptions();

    public bool LoadStandardLibraries { get; }
    public bool EntryPointEnabled { get; }
    public string? DefaultNamespace { get; }
    public ImmutableArray<string> LibraryPaths { get; }
    public ImmutableArray<string> SourcePaths { get; }

    private ScriptOptions(
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

    public ScriptOptions WithStandardLibraries()
        => new(true, EntryPointEnabled, DefaultNamespace, LibraryPaths, SourcePaths);

    public ScriptOptions WithNamespace(string ns)
    {
        if (ns == null)
            throw new ArgumentNullException(nameof(ns));

        return new(LoadStandardLibraries, EntryPointEnabled, ns, LibraryPaths, SourcePaths);
    }

    public ScriptOptions WithLibrary(string path)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        return new(LoadStandardLibraries, EntryPointEnabled, DefaultNamespace, LibraryPaths.Add(path), SourcePaths);
    }

    public ScriptOptions WithSourceFile(string path)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        return new(LoadStandardLibraries, EntryPointEnabled, DefaultNamespace, LibraryPaths, SourcePaths.Add(path));
    }

    public ScriptOptions WithEntryPoint(bool enabled)
        => new(LoadStandardLibraries, enabled, DefaultNamespace, LibraryPaths, SourcePaths);
}
