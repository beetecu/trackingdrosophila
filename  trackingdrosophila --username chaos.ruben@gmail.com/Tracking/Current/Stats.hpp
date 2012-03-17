/*
 * Stats.hpp
 *
 *  Created on: 31/01/2012
 *      Author: chao
 */

#ifndef STATS_HPP_
#define STATS_HPP_

#include "Libreria.h"

void CalcStatsFrame( STFrame* frameDataStats,STFrame* frameDataOut );

STStatFrame* InitStatsFrame( int NumFrame, timeval tif, timeval tinicio, int TotalFrames, float FPS  );

#endif /* STATS_HPP_ */
