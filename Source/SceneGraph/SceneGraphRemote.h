#pragma once
#include "SceneGraph.h"

namespace sg {

void SetUSDModuleDir(const std::string& v);
void SetUSDPluginPath(const std::string& v);
ScenePtr CreateUSDScene();
ScenePtr CreateUSDScenePipe();

} // namespace sg
