#include <opencv2/opencv.hpp> 
#include <iostream>
#include <math.h>

using namespace cv;
using namespace std;

int main(int argc,char** argv)
{
	Mat src,src_gray, src_thrd;
	src=imread("../../1.jpg");             //读取并显示图像
	if(src.empty())
	{
		printf("could not load image...\n");
		return -1;
	}
  
	namedWindow("img",CV_WINDOW_FREERATIO);            
	imshow("img",src);                             //-
	cvtColor(src, src_gray, CV_BGR2GRAY);
	threshold(src_gray, src_thrd, 198, 255, THRESH_BINARY);
//***************获取边缘点阵**********
	Mat src_ep(src.size(), CV_8UC1, Scalar(0));
	for (int row = 1; row <src_thrd.rows-1 ; row++)
	{
		uchar* datap = src_thrd.ptr<uchar>(row-1);
		uchar* datac = src_thrd.ptr<uchar>(row);
		uchar* datan = src_thrd.ptr<uchar>(row+1);
		uchar* data  = src_ep  .ptr<uchar>(row);
		for (int col = 1; col < src_thrd.cols-1; col++)
		{
			if(datac[col]   == 255) continue;
			if(datap[col]   == datan[col]) continue;
			if(datac[col-1] == datac[col+1]) continue;
			data[col]=255;
		}
	}
	//imwrite("D:/器件识别/图4边缘点阵.jpg",src_ep);

//*******************通过点阵初步提取器件**************
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3));
	for(int i=0; i<9; i++)
	//for(int i=0; i<5; i++)
	{
		dilate(src_ep, src_ep, kernel, Point(-1,-1));
	}
	//imwrite("D:/器件识别/图5边缘点阵膨胀.jpg",src_ep);
  	Mat src_q(src.size(), CV_8UC1, Scalar(255));  //利用点阵特征提取
	for(int row=0; row<src_ep.rows; row++)
	{
		for(int col=0; col<src_ep.cols; col++)
		{
			uchar* dataep = src_ep.ptr<uchar>(row);
			uchar* dataq = src_q.ptr<uchar>(row);
			uchar* datas = src_gray.ptr<uchar>(row);
			if(dataep[col]!=255) continue;
			dataq[col] = datas[col];
		}
	}
	//imwrite("D:/器件识别/图6器件提取.jpg",src_q);

//******************利用轮廓进一步去除不相关部分*****************

	Mat src_qa;
	threshold(src_q, src_qa, 140, 255, THRESH_BINARY);
	bitwise_not(src_qa, src_qa);
	vector<vector<Point>> contours;
	vector<Vec4i> h;
	findContours(src_qa, contours, h, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point());
	vector<Rect> rect(contours.size());
	vector<Rect> rectw;//(contours.size());
	src_qa.setTo(255);
	for(int i=0;i<contours.size();i++)		//绘制器件轮廓
	{
		rect[i]=boundingRect(contours[i]);
		int w=rect[i].width, h=rect[i].height;
		//if(w<9 || h<9 || (w*h)<180)
		if(w<6 || h<6 || (w*h)<180)
		{
			rectw.push_back(rect[i]);
			continue;
		}
		rectangle(src_qa, Point(rect[i].x, rect[i].y), Point(rect[i].x+w, rect[i].y+h), Scalar(0), -1, 8);
	}
	//printf("%d\n",rectw.size());
	for(int i=0; i<rectw.size();i++)
		rectangle(src_qa, Point(rectw[i].x, rectw[i].y), Point(rectw[i].x+rectw[i].width, rectw[i].y+rectw[i].height), Scalar(255), -1, 8);

	Mat src_sq (src.size(), CV_8UC1, Scalar(255));
	for(int row=0; row<src_qa.rows; row++)			//元件提取
	{
		uchar* data1 = src_qa.ptr<uchar>(row);
		uchar* data = src_gray.ptr<uchar>(row);
		uchar* data2 = src_sq.ptr<uchar>(row);
		for(int col=0; col<src_qa.cols; col++)
		{
			if(data1[col]==0)
				data2[col]=data[col];
		}
	}
	namedWindow("O1img",CV_WINDOW_FREERATIO);            
	imshow("O1img",src_sq); 
	imwrite("../../器件进一步提取.jpg",src_sq);
	waitKey(0);
	return 0;
}