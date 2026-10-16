using UnrealBuildTool;
public class MyProject : ModuleRules {
 public MyProject(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","UMG","Slate","SlateCore","Json","AudioMixer"});
  if(Target.Platform==UnrealTargetPlatform.Android)
   AdditionalPropertiesForReceipt.Add("AndroidPlugin",System.IO.Path.Combine(ModuleDirectory,"S2Offline_UPL.xml"));
 }
}
