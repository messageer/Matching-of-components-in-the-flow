#include <opencv2/opencv.hpp> 
#include <iostream>
#include <math.h>

using namespace cv;
using namespace std;

RNG rng(12345);
int blue;
int green;
int red;

Mat src, src_gray, src_thrd, src_thrdn;
Mat temp, temp_gray, temp_thrd;


void ex_circle(vector<vector<Point>> contours, vector<vector<uchar>>&v_circle, vector<uchar>&flag)   ; 
void getpoint( vector< vector<int> >&blackpoint, vector< vector<int> >&blackpointt, uint &sum);
void match_in_coutours(vector<vector<Point>>&contours, vector<vector<int>>&blackpoint, vector<vector<int>>&blackpointt,
								uint &sum);

int main(int argc,char** argv)
{
	src = imread("../../器件进一步提取.jpg");             //读取并显示图像

	if(src.empty())
	{
		printf("could not load image...\n");
		return -1;
	}
	namedWindow("img",CV_WINDOW_FREERATIO); 
	namedWindow("oimg",CV_WINDOW_FREERATIO); 
//	namedWindow("oimg1",CV_WINDOW_FREERATIO); 
	cvtColor(src, src_gray, CV_BGR2GRAY);
	threshold(src_gray, src_thrd, 193, 255, THRESH_BINARY);
	imshow("img", src);

//*************************轮廓检测***************************
	bitwise_not(src_thrd, src_thrdn);
	vector<vector<Point>> contours;
	vector<Vec4i> h;
	findContours(src_thrdn, contours, h, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point());

//************************圆检测**************************
	vector<vector<uchar>> v_circle;
	//vector<Point2i> p_circle;
	vector<uchar> flag;
	ex_circle(contours, v_circle, flag);
	for (int i = 0; i < v_circle.size(); i++)
	{		
		blue = rng.uniform(0,255);
		green = rng.uniform(0,255);
		red = rng.uniform(0,255);
		for (int j = 0; j < v_circle[i].size(); j++)
		{
			Rect rect = boundingRect(contours[v_circle[i][j]]);
			int w=rect.width, h=rect.height;
			int x=rect.x, y=rect.y;
			rectangle(src, Rect(x,y,w,h), Scalar(blue, green, red), 3);
		}
	}

//************************其他匹配**********************************	
	string pattern_jpg = "../../device/*.jpg";    //读取文件夹内模板
	vector<String> image_files;
	glob(pattern_jpg, image_files);
	for (int i = 0; i < image_files.size(); i++)
	{
		temp = imread(image_files[i]);
		//imshow("img", temp);
		cvtColor(temp, temp_gray, CV_BGR2GRAY);

		vector<vector<int>> blackpoint;
		vector<vector<int>> blackpointt;
		uint sum;
		getpoint(blackpoint, blackpointt, sum);
		
		//Mat test(temp_gray.size(), CV_8UC1, Scalar(255));
		//for (int row = 0; row < blackpoint.size(); row++)
		//{
		//	uchar* data = test.ptr<uchar>(row);
		//	for (int col= 0; col < blackpoint[row].size(); col++)
		//	{
		//		data[ blackpoint[row][col] ] = 0;
		//	}
		//}
		//imshow("oimg", test);
		//waitKey(0);

		match_in_coutours(contours, blackpoint, blackpointt, sum);

		//imshow("oimg", src);
		//waitKey(0);
	}

	imshow("oimg", src);
	imwrite("../../最终匹配.jpg",src);
	waitKey(0);
	return 0;
}

void ex_circle(vector<vector<Point>> contours, vector<vector<uchar>>&v_circle, vector<uchar>&flag)          
{
	v_circle.resize(6);
	for (int i = 0; i < contours.size(); i++)
	{
		Mat mask, maskt;
		Rect rect = boundingRect(contours[i]);
		int w=rect.width, h=rect.height;
		int x=rect.x, y=rect.y;
		if(w<20 || h<20) continue;
		src_thrd( Rect(x, y, w, h) ).copyTo(mask);
		src_gray( Rect(x, y, w, h) ).copyTo(maskt);

		vector<Vec3f> circles;
		HoughCircles(maskt, circles, HOUGH_GRADIENT, 1, 30,   80, 25,   8, 60);
		if(circles.size()<=0) continue;


		uchar rs, cs, rf, cf, radius, fg;
		radius = circles[0][2];
		rf = 2; cf = radius * 0.6;
		rs = circles[0][1] - rf; 
		cs = circles[0][0] - cf;

//*****************1、2、3的区分*******************
		uchar f1=0;
		for (int row = 0; row < rf*2+1; row++)
		{
			uchar f=1;
			uchar* data = mask.ptr<uchar>(rs+row);
			if(data[cs]==0) f=0;
			for (int col = 0; col < cf*2+1; col++)
			{
				if(f==1 && data[cs+col]==0) goto loop2;
				if(f==0 && data[cs+col]==255) goto loop2;
			}
			if(f==0)	{	v_circle[1].push_back(i); goto loop;	}
			if(f==1)	f1=1;		//由两种情况未分辨
loop2: ;
		}
		if(f1==1) 
		{
			uchar* data = mask.ptr<uchar>(circles[0][1] - radius*0.4);
			for (int col = 0; col < cf*2+1; col++)
			{
				if(data[cs+col] == 0)	{	v_circle[3].push_back(i); goto loop;	}
			}
			v_circle[0].push_back(i); goto loop;
		}
//****************4和5的区分*****************
		for (int row = 0; row < cf*2+1; row++)
		{
			if(mask.at<uchar>(circles[0][1] - cf + row, cs+3) == 0)
			{
				if(row<(cf*2+1)/3)		{	v_circle[5].push_back(i); goto loop;	}
				else if(row>(cf*4+2)/3) {	v_circle[4].push_back(i); goto loop;	}
				else					{	v_circle[2].push_back(i); goto loop;	}
			}
		}

		//imshow("img", mask);
		//waitKey(0);
loop: ;
	}
}  

void getpoint( vector< vector<int> >&blackpoint, vector< vector<int> >&blackpointt, uint &sum)
{
	sum=0;
	blackpoint.resize(temp_gray.rows);
	blackpointt.resize(temp_gray.cols);
	for(int row=0; row<temp_gray.rows; row++)
	{
		uchar *data = temp_gray.ptr<uchar>(row);
		for(int col=0; col<temp_gray.cols; col++)
		{
			if(data[col]<125)                 
			{
				blackpoint[row].push_back(col);
				blackpointt[col].push_back(row);
				sum++;
			}
		}
	} 

}

void match_in_coutours(vector<vector<Point>>&contours, vector<vector<int>>&blackpoint, vector<vector<int>>&blackpointt,
								uint &sum)
{
//****************在各个轮廓内匹配*******************
	vector<Rect> rect(contours.size());
	Mat mask;
	int rows=src.rows-temp_gray.rows+1;
	int cols=src.cols-temp_gray.cols+1;
	//Mat result (src.size(), CV_32FC1, Scalar(4000));
	Mat result (src.size(), CV_32FC1, Scalar(500));
	Mat resultt(src.size(), CV_8UC1, Scalar(0));
	uint sum_b;
	float fbrk;
	//Mat src_grayt,src_thrdt;
	//cvtColor(src, src_grayt, CV_BGR2GRAY);
	//threshold(src_grayt, src_thrdt, 140, 255, THRESH_BINARY);

	for (int i = 0; i < contours.size(); i++)
	{
		rect[i]=boundingRect(contours[i]);
		int w=rect[i].width, h=rect[i].height;
		int x=rect[i].x, y=rect[i].y;
		//src_grayt( Rect(x, y, w, h) ).copyTo(mask);
		src_thrd( Rect(x, y, w, h) ).copyTo(mask);

		//fbrk=sum[j]*2800.0;
		fbrk=sum*255*0.3;
		if(w>=temp_gray.cols && h>=temp_gray.rows )
		{
			for (int row = 0; row < mask.rows-temp_gray.rows+1; row++)
			{
				for(int col = 0; col < mask.cols-temp_gray.cols+1; col++)
				{
					sum_b=0;
					for (int r = 0; r < temp_gray.rows; r++)					//模板内循环
					{
						uchar* data = mask.ptr<uchar>(row+r);
						for (int bp = 0; bp < blackpoint[r].size (); bp++)
						{
							sum_b += data[ col + blackpoint[r][bp] ] ;
						}
						if(sum_b>fbrk)  goto loop1;
					}
					//printf("%5d\t",sum_b);
					result.at<float>(y+row, x+col) = (float)sum_b/sum;
					//printf("%4.2f\n",result.at<float>(y+row, x+col));
					resultt.at<uchar>(y+row, x+col) = 1;
					loop1: ;
				}
			}
		}
//**********************旋转90°后匹配*********************
			if(w>=temp_gray.rows && h>=temp_gray.cols )
			{
				for (int row = 0; row < mask.rows-temp_gray.cols+1; row++)
				{
					for(int col = 0; col < mask.cols-temp_gray.rows+1; col++)
					{
						sum_b=0;
						for (int r = 0; r < temp_gray.cols; r++)					//模板内循环
						{
							uchar* data = mask.ptr<uchar>(row+r);
							for (int bp = 0; bp < blackpointt[r].size (); bp++)
							{
								sum_b += data[ col + blackpointt[r][bp] ] ;
							}
							if(sum_b>fbrk)  goto loop2;
						}
						//printf("%5d\t",sum_b);
						result.at<float>(y+row, x+col) = (float)sum_b/sum;
						//printf("%4.2f\n",result.at<float>(y+row, x+col));
						resultt.at<uchar>(y+row, x+col) = 2;
						loop2: ;
					}
				}				
			}
//*****************
	}

//**********************绘制匹配图************************
	printf("*******************END*************\n");
	uint a=0,a1=0,b=0,b1=0;				//储存
	uchar t;
//	RNG rng(12345);
	int blue = rng.uniform(0,255);
	int green = rng.uniform(0,255);
	int red = rng.uniform(0,255);
	int w=temp_gray.cols, h=temp_gray.rows;
	float mx=0;
	for(int row=0; row<result.rows; row++)
	{
		float* data  = result.ptr<float>(row);
		uchar* datat = resultt.ptr<uchar>(row);
		for(int col=0; col<result.cols;col++)
		{
			if( data[col] <  255*0.3 )
			//if( data[col] <  255*0.25 )
			{
				a=row; b=col;		  //记录第一个满足的点
		   loop3:while(a!=a1 || b!=b1)
				{
					a1=a;b1=b;		  //保存前一组点
					for(int i=0;i<3;i++)		   //找3*3区域最小值
					{                              
						for(int j=0;j<3;j++)
						{
							if( result.at<float>(a1-1+i,b1-1+j)<result.at<float>(a,b) )
							{
								a=a1-1+i;b=b1-1+j;							//a1,b1为实时最小值对应行列
								goto loop3;
							}
						}
					}
				}
				 if(datat[col]%10 ==1){
					 rectangle(src, Rect(b, a, w, h), Scalar(blue,green,red), 2, 8);
					 rectangle(src_thrd, Rect(b, a, w, h), Scalar(255), -1, 8);
					 printf("%f\n",result.at<float>(a,b));
				 }
				 else{
					 rectangle(src, Rect(b, a, h, w), Scalar(blue,green,red), 2, 8);
					 rectangle(src_thrd, Rect(b, a, h, w), Scalar(255), -1, 8);
					 printf("%f\n",result.at<float>(a,b));
				 }
			}
		}
	}
	//printf("%d\t",temp_p[0].rows*temp_p[0].cols);
	//printf("max is %f\n",mx);
}



