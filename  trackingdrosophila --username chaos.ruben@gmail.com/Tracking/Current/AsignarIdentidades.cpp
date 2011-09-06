/*
 * AsignarIdentidades.cpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#include "AsignarIdentidades.hpp"

void MotionTemplate( IplImage* img, IplImage* dst){

	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	CvSize size = cvSize(img->width,img->height); // get current frame size
	int i, idx1 = last, idx2;

	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;

	if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
//	        if( buf == 0 ) {
//	            buf = (IplImage**)malloc(N*sizeof(buf[0]));
//	            memset( buf, 0, N*sizeof(buf[0]));
//	        }
//	        for( i = 0; i < N; i++ ) {
//	            cvReleaseImage( &buf[i] );
//	            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
//	            cvZero( buf[i] );
//	        }
	        cvReleaseImage( &mhi );
	        cvReleaseImage( &orient );
	        cvReleaseImage( &segmask );
	        cvReleaseImage( &mask );

	        silh = cvCreateImage( size, 8, 1 );
	        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        cvZero( mhi ); // clear MHI at the beginning
	        silh = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	    }
//	cvCopy( img, buf[last]);
//	idx2 = (last + 1) % N; // index of (last - (N-1))th frame
//	last = idx2;
	cvCopy( img, silh);
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,(MHI_DURATION - timestamp)*255./MHI_DURATION );
	cvZero( dst );
	cvMerge( mask, 0, 0, 0, dst );
   // calculate motion gradient orientation and valid orientation mask
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );

	// iterate through the motion components,
	// One more iteration (i == -1) corresponds to the whole image (global motion)
	for( i = -1; i < seq->total; i++ ) {

		if( i < 0 ) { // case of the whole image
			comp_rect = cvRect( 0, 0, size.width, size.height );
			color = CV_RGB(255,255,255);
			magnitude = 100;
		}
		else { // i-th motion component
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
//			if( comp_rect.width + comp_rect.height < 100 ) // reject very small components
//				continue;
			color = CV_RGB(255,0,0);
			magnitude = 30;


		// select component ROI
		cvSetImageROI( silh, comp_rect );
		cvSetImageROI( mhi, comp_rect );
		cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );

		// calculate orientation
		angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
		angle = 360.0 - angle;  // adjust for images with top-left origin

		count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.05 )
			continue;

		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),
						  (comp_rect.y + comp_rect.height/2) );

		cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
				cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
//		cvWaitKey(0);
		}
	}
}

