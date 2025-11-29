/**
 * @file subcollider.h
 * @brief Main include header for SubCollider DSP engine.
 *
 * Include this header to access all SubCollider components.
 */

#ifndef SUBCOLLIDER_H
#define SUBCOLLIDER_H

// Core types and utilities
#include "subcollider/types.h"
#include "subcollider/AudioBuffer.h"
#include "subcollider/AudioLoop.h"

// UGens
#include "subcollider/ugens/SinOsc.h"
#include "subcollider/ugens/SawDPW.h"
#include "subcollider/ugens/LFTri.h"
#include "subcollider/ugens/EnvelopeAR.h"
#include "subcollider/ugens/EnvelopeADSR.h"
#include "subcollider/ugens/LFNoise2.h"
#include "subcollider/ugens/Pan2.h"
#include "subcollider/ugens/Balance2.h"
#include "subcollider/ugens/Lag.h"
#include "subcollider/ugens/XLine.h"
#include "subcollider/ugens/Downsampler.h"

// Moog Ladder Filters
#include "subcollider/ugens/StilsonMoogLadder.h"
#include "subcollider/ugens/MicrotrackerMoogLadder.h"
#include "subcollider/ugens/KrajeskiMoogLadder.h"
#include "subcollider/ugens/MusicDSPMoogLadder.h"
#include "subcollider/ugens/OberheimMoogLadder.h"
#include "subcollider/ugens/ImprovedMoogLadder.h"
#include "subcollider/ugens/RKSimulationMoogLadder.h"

// Composite UGens
#include "subcollider/ugens/SuperSaw.h"

// Example voice
#include "subcollider/ExampleVoice.h"

#endif // SUBCOLLIDER_H
