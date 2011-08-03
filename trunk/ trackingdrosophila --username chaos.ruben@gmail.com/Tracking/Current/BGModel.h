/*
 * BGModel.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef BGMODEL_H_
#define BGMODEL_H_

#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>

IplImage* updateBackground(CvBGStatModel * bg_model, IplImage * tmp_frame);

void initBackgroundModel(CvBGStatModel ** , IplImage* , CvGaussBGStatModelParams* );

IplImage * getBinaryImage(IplImage * image);

#endif /* BGMODEL_H_ */
