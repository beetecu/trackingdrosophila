/*
 * AsignarIdentidades.cpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#include "AsignarIdentidades.hpp"

// various tracking parameters (in seconds)
const double MHI_DURATION = 0.5;//0.3
const double MAX_TIME_DELTA = 0.5;	//0.5
const double MIN_TIME_DELTA = 0.05;	//0.05
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
//IplImage **buf = 0;
//int last = 0;

// temporary images
IplImage* img;
IplImage* silh; // Imagen con las siluetas
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map

void MotionTemplate( tlcde* framesBuf){

	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds

	int i, idx2;
//	int  idx1 = last;
	CvMemStorage* storage = 0; // temporary storage
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;
	STFrame* frameData;

	STFly* fly;

	frameData = ( STFrame* )obtenerActual( framesBuf );
	CvSize size = cvSize(frameData->FG->width,frameData->FG->height); // get current frame size
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
		    cvReleaseImage( &img );
	        cvReleaseImage( &mhi );
	        cvReleaseImage( &orient );
	        cvReleaseImage( &segmask );
	        cvReleaseImage( &mask );
	        cvReleaseImage( &silh);

	        img =  cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        cvZero( mhi ); // clear MHI at the beginning
	        silh = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	    }

	img = frameData->FG;
//	cvZero(img);
	i = 0;

	// obtener máscara del fg con rectángulos en ved de elipses,

//	while( i <  frameData->Flies->numeroDeElementos ){
//		fly = (STFly*)obtener(i, frameData->Flies);
//		cvSetImageROI( img, fly->Roi);
//		invertirBW( img ); // añadir para invertir con roi
//		cvResetImageROI( img);
//		i++;
//	}
//	cvShowImage( "Motion",img);
//	cvWaitKey(0);

//	cvCopy( img, buf[last]);
//	idx2 = (last + 1) % N; // index of (last - (N-1))th frame
//	last = idx2;
	cvCopy( img, silh);
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,(MHI_DURATION - timestamp)*255./MHI_DURATION );
	cvZero( frameData->ImMotion );
	cvMerge( mask, 0, 0, 0, frameData->ImMotion );
   // calculate motion gradient orientation and valid orientation mask
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 ); //0.01, 0.5

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );
//	cvShowImage( "Motion",segmask);
//			cvWaitKey(0);
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
//						cvShowImage( "Motion",orient);
//								cvWaitKey(0);
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

		// Resolvemos ambiguedad en orientación y etiquetamos
		// buscamos el centro del blob que se corresponde con la roi y su silueta

//		if (framesBuf->numeroDeElementos == 1) etiquetar();
//		for( int y=comp_rect.y;  y< comp_rect.y+comp_rect.height; y++){
//			uchar* ptr = (uchar*) ( silh->imageData + y*silh->widthStep + 1*comp_rect.x);
//			for (int x = 0; x<comp_rect.width; x++) {
//				if (ptr[x] == 255){
//					i = 0;
//					while( i <  frameData->Flies->numeroDeElementos ){
//							fly = (STFly*)obtener(i, frameData->Flies);
//							if ( (fly->posicion.x == x)&&(fly->posicion.y == y) ) break;
//							i++;
//						}
//					if ( (fly->posicion.x == x)&&(fly->posicion.y == y) ) break;
//				}
//			}
//		}


		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),
						  (comp_rect.y + comp_rect.height/2) );

		cvCircle( frameData->ImMotion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		cvLine( frameData->Frame, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
				cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
		imshow( "Motion",frameData->ImMotion);
//					cvWaitKey(0);

		}
	}
}



tlcde* matchingIdentity( tlcde* framesBuf ,int estado ){

	tlcde* FliesFrActual;
	tlcde* FliesFrSig;



	if ( estado == 1){

	}
	else{

	}
}

void releaseMotionTemplate(){
	cvReleaseImage( &img );
	cvReleaseImage( &mhi );
	cvReleaseImage( &orient );
	cvReleaseImage( &segmask );
	cvReleaseImage( &mask );
	cvReleaseImage( &silh);
}
