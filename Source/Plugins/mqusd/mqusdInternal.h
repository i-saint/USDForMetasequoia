#pragma once
#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

namespace mqusd {

class mqusdExporterPlugin;
class mqusdExporterWindow;
class mqusdImporterPlugin;
class mqusdImporterWindow;
class mqusdRecorderPlugin;
class mqusdRecorderWindow;

void mqusdInitialize();
void mqusdFinalize();

} // namespace mqusd
