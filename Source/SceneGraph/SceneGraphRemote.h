#pragma once
#include "SceneGraph.h"

namespace sg {

bool LoadUSDModule();
void SetUSDModuleDir(const std::string& v);

ScenePtr CreateUSDScene();
ScenePtr CreateUSDScenePipe();

} // namespace sg
