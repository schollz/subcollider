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
#include "subcollider/ugens/EnvelopeAR.h"
#include "subcollider/ugens/LFNoise2.h"

// Example voice
#include "subcollider/ExampleVoice.h"

#endif // SUBCOLLIDER_H
