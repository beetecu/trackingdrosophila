/*
 * VideoTracker.hpp
 *
 *  Created on: 12/08/2011
 *      Author: chao
 */

#ifndef VIDEOTRACKER_HPP_
#define VIDEOTRACKER_HPP_ //Modifica

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <opencv2/video/background_segm.hpp>

#define CVX_RED CV_RGB(255,0,0)
#define CVX_BLUE CV_RGB(0,0,255)
#define CVX_GREEN CV_RGB(0,255,0)
#define CVX_WHITE CV_RGB(255,255,255)
#define CVX_BLACK CV_RGB(0,0,0)

//Opciones de visualización
#define CREATE_TRACKBARS 1
#define SHOW_BG_REMOVAL 1 ///<- switch from 0 to 1 para visualizar background
#define SHOW_VISUALIZATION 0 ///<- switch from 0 to 1 para visualizar resultado

#endif /* VIDEOTRACKER_HPP_ */
