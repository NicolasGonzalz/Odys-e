// Module OdysseeNet : embarque shared/ (réseau, simulation) dans Unreal (ADR 0003).
using System.IO;
using System.Text.RegularExpressions;
using UnrealBuildTool;

public class OdysseeNet : ModuleRules
{
	public OdysseeNet(ReadOnlyTargetRules Target) : base(Target)
	{
		// Les sources de shared/ sont compilées une par une (Private/Shared/, générées par
		// scripts/odysseenet_sources.py) : sans PCH ni unity build, les macros d'Unreal (check,
		// verify…) ne se mélangent pas au code partagé, et chaque fichier reste isolé.
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DeveloperSettings" });

		string repoRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", "..", ".."));
		string shared = Path.Combine(repoRoot, "shared");
		PublicIncludePaths.Add(Path.Combine(shared, "include")); // #include "odyssee/…"
		PrivateIncludePaths.Add(shared);                         // les enveloppes : #include "src/…"
		PrivateDefinitions.Add("ODYSSEE_VERSION_STRING=\"" + ProjectVersion(repoRoot) + "\"");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("ws2_32.lib"); // Winsock, comme dans shared/CMakeLists.txt
		}
	}

	// Même version que le CMake du dépôt (project(Odyssee VERSION x.y.z)) : une seule source.
	private static string ProjectVersion(string repoRoot)
	{
		string cmake = File.ReadAllText(Path.Combine(repoRoot, "CMakeLists.txt"));
		Match match = Regex.Match(cmake, @"project\(\s*Odyssee\s+VERSION\s+([0-9.]+)");
		return match.Success ? match.Groups[1].Value : "inconnue";
	}
}
