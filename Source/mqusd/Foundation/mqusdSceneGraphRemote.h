#pragma once
#include "mqusdSceneGraph.h"

namespace mqusd {

void SetUSDModuleDir(const std::string& v);
void SetUSDPluginPath(const std::string& v);
ScenePtr CreateUSDScene();
ScenePtr CreateUSDRemoteScene();

SceneInterface* CreateUSDSceneInterface(Scene* scene);
SceneInterface* CreateUSDScenePipe(Scene* scene);

} // namespace mqusd
