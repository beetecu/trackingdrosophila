/*
 * BGModel.cpp
 *
 * Modelado de fondo mediante distribución Gaussiana
 *
 * Recibe un puntero tipo cv_Capture a la secuencia a modelar
 * Devuelve un puntero IplImage al modelo de fondo
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */


#include "BGModel.h"

void initBackgroundModel(CvBGStatModel ** bgmodel, IplImage* tmp_frame, CvGaussBGStatModelParams* paramMoG){

        paramMoG->win_size = CV_BGFG_MOG_WINDOW_SIZE; //200;
        paramMoG->n_gauss = 5; //5;
        paramMoG->bg_threshold = 0.1; //0.7;
        paramMoG->std_threshold = 5; //2.5;
        paramMoG->minArea = 200.f; //15.f;
        paramMoG->weight_init = 0.01; //0.05;
        paramMoG->variance_init = 30; //30*30;
        *bgmodel = cvCreateGaussianBGModel(tmp_frame, paramMoG);

}


///The function that make the Background subtraction with Gaussian model
/**
 * \param aviName the name of the avi video to process
 * \return savedBackgroundImage the background of the video
 */

IplImage* updateBackground(CvBGStatModel *bg_model, IplImage * tmp_frame){

        //Updating the Gaussian Model
        cvUpdateBGStatModel(tmp_frame, bg_model);
  // if(!cvSaveImage("./data/background.jpg",bg_model->background)) printf("Could not save the background image\n");
//if(!cvSaveImage("./data/foreground.jpg",bg_model->foreground)) printf("Could not save the foreground image\n");

        //returing the binary foreground
        return bg_model->foreground;
}

IplImage * getBinaryImage(IplImage * image){


        //!getting the binary current frame
        IplImage* img = cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);
        cvCvtColor(image, img, CV_RGB2GRAY);
        IplImage* binImg = cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);
        cvThreshold(img,binImg,100,255,CV_THRESH_BINARY);
        //if(!cvSaveImage("binImg.jpg",binImg)) printf("Could not save the backgroundimage\n");
        cvReleaseImage(&img);
        return binImg;

}









