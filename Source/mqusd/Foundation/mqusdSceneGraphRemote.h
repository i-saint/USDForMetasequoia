#pragma once
#include "mqusdSceneGraph.h"

namespace mqusd {

void SetUSDModuleDir(const std::string& v);
void SetUSDPluginPath(const std::string& v);
ScenePtr CreateUSDScene();
ScenePtr CreateUSDScenePipe();

} // namespace mqusd
