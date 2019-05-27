/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_scripting
  vendor:           Hart Instruments
  version:          2.0.0
  name:             HISE Scripting Module
  description:      The scripting engine module for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp, hi_components, hi_dsp_library, hi_sampler

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once

#define INCLUDE_TCC 0

#define MAX_SCRIPT_HEIGHT 700

#define INCLUDE_NATIVE_JIT 0

#include "AppConfig.h"
#include "../hi_sampler/hi_sampler.h"
#include "../hi_dsp_library/hi_dsp_library.h"

#if INCLUDE_NATIVE_JIT
#include "../hi_native_jit/hi_native_jit_public.h"
#endif



#include "scripting/api/ScriptMacroDefinitions.h"
#include "scripting/engine/JavascriptApiClass.h"
#include "scripting/api/ScriptingBaseObjects.h"

#if JUCE_IOS
#elif INCLUDE_TCC
#include "scripting/api/TccContext.h"
#endif

//#include "scripting/api/DspFactory.h"
#include "scripting/engine/DebugHelpers.h"
#include "scripting/api/DspInstance.h"


#include "scripting/scripting_audio_processor/ScriptDspModules.h"

#include "scripting/scriptnode/data/Properties.h"
#include "scripting/scriptnode/data/RangeHelpers.h"
#include "scripting/scriptnode/data/ValueTreeHelpers.h"
#include "scripting/scriptnode/data/DspHelpers.h"
#include "scripting/scriptnode/data/NodeBase.h"
#include "scripting/scriptnode/data/CodeGenerator.h"
#include "scripting/scriptnode/data/DspNetwork.h"
#include "scripting/scriptnode/nodewrappers/Base.h"
#include "scripting/scriptnode/nodewrappers/Bypass.h"

#include "scripting/scriptnode/nodewrappers/Containers.h"
#include "scripting/scriptnode/nodewrappers/Processors.h"

#include "scripting/scriptnode/data/StaticNodeWrappers.h"
#include "scripting/scriptnode/data/ModulationSourceNode.h"
#include "scripting/scriptnode/data/NodeContainer.h"
#include "scripting/scriptnode/data/NodeWrapper.h"
#include "scripting/scriptnode/data/ProcessNodes.h"
#include "scripting/scriptnode/data/DspNode.h"
#include "scripting/scriptnode/data/FeedbackNode.h"


#include "scripting/scriptnode/ui/ParameterSlider.h"
#include "scripting/scriptnode/ui/PropertyEditor.h"
#include "scripting/scriptnode/ui/NodeComponent.h"
#include "scripting/scriptnode/ui/ModulationSourceComponent.h"
#include "scripting/scriptnode/ui/NodeContainerComponent.h"
#include "scripting/scriptnode/ui/FeedbackNodeComponents.h"
#include "scripting/scriptnode/ui/DspNodeComponent.h"
#include "scripting/scriptnode/ui/DspNetworkComponents.h"


#include "scripting/engine/HiseJavascriptEngine.h"

#include "scripting/api/XmlApi.h"
#include "scripting/api/ScriptingApiObjects.h"
#include "scripting/api/ScriptingApi.h"
#include "scripting/api/ScriptingApiContent.h"
#include "scripting/api/ScriptComponentEditBroadcaster.h"



#include "scripting/ScriptProcessor.h"
#include "scripting/ScriptProcessorModules.h"
#include "scripting/HardcodedScriptProcessor.h"
#include "scripting/hardcoded_modules/Arpeggiator.h"

#include "scripting/api/ScriptComponentWrappers.h"
#include "scripting/components/ScriptingContentComponent.h"



#if USE_BACKEND

#include "scripting/components/ScriptingPanelTypes.h"
#include "scripting/components/PopupEditors.h"
#include "scripting/components/ScriptingCodeEditor.h"
#include "scripting/components/AutoCompletePopup.h"
#include "scripting/components/ScriptingContentOverlay.h"
#include "scripting/components/ScriptingEditor.h"

#endif 





