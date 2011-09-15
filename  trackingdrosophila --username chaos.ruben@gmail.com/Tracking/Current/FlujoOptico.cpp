/*
 * FlujoOptico.cpp
 *
 *  Created on: 14/09/2011
 *      Author: chao
 */

#include "VideoTracker.hpp"
#include "opencv2/video/tracking.hpp"

IplImage **buf = 0;
int N = 2; // tamaño del bufer
int last = N-1;
int prev = N-2;

void OpticalFlowLK( IplImage* ImLast, IplImage* velX, IplImage* velY);

void OpticalFlowLK( IplImage* ImLast,IplImage* velX,IplImage* velY ){

	CvSize size = cvSize(ImLast->width,ImLast->height); // get current frame size


	if( buf == 0 ) {
		buf = (IplImage**)malloc(N*sizeof(buf[0]));
		memset( buf, 0, N*sizeof(buf[0]));

		for( int i = 0; i < N; i++ ) {
			buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
			cvZero( buf[i] );
		}
		cvCopy(ImLast, buf[last]);
		return;
	}
	cvCopy( buf[last], buf[prev]);
	cvCopy( ImLast, buf[last]);

	cvCalcOpticalFlowLK(buf[prev], buf[last], cvSize(3, 3), velX, velY);

}
