#pragma once

#include "MQPlugin.h"
#include "MQBasePlugin.h"
#if MQPLUGIN_VERSION > 0x0400
    #include "MQWidget.h"
#endif
#if MQPLUGIN_VERSION >= 0x0464
    #include "MQBoneManager.h"
#endif
#if MQPLUGIN_VERSION >= 0x0470
    #include "MQMorphManager.h"
#endif
