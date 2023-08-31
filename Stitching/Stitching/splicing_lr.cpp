

#include <iostream>
#include <opencv2/opencv.hpp>
#include <time.h>
#include "opencv2/imgproc/imgproc_c.h"
#include<math.h>
#include <numeric>
using namespace std;
using namespace cv;

using namespace std;
using namespace cv;

#include <iostream>
#include <opencv2/opencv.hpp>
#include <time.h>
#include "opencv2/imgproc/imgproc_c.h"
using namespace std;
using namespace cv;

using namespace std;
using namespace cv;

void light(Mat image1, Mat image2)
{
	// ��������ͼ���ƽ������ֵ
	Scalar mean1 = mean(image1);
	Scalar mean2 = mean(image2);

	// �������Ȳ���ֵ
	double brightness_diff = mean1[0] - mean2[0];

	// �����ڶ���ͼ�������ֵ
	image2.convertTo(image2, -1, 1, brightness_diff);

	// ��ʾ�������ͼ��
	/*imshow("Image1", img1);
	imshow("Image2", img2);
	waitKey(0);*/

}

void light2(Mat image1, Mat image2)
{

	const int histSize[] = { 256 };
	float pranges[] = { 0, 255 };//ȡֵ����
	const float* histRange[] = { pranges };
	// ��image1ת��Ϊ�Ҷ�ͼ��
	cvtColor(image1, image1, COLOR_BGR2GRAY);

	// ����image2��ֱ��ͼ
	Mat hist2;
	calcHist(&image2, 1, 0, Mat(), hist2, 1, histSize, histRange, true, false);

	// ���⻯image1��ֱ��ͼ
	equalizeHist(image1, image1);

	// ������⻯���image1��ֱ��ͼ
	Mat hist1;
	calcHist(&image1, 1, 0, Mat(), hist1, 1, histSize, histRange, true, false);


}


//ͼ���ںϵ�ȥ�ѷ촦������
bool OptimizeSeam(int start_x, int end_x, Mat& WarpImg, Mat& DstImg)
{
	double Width = (end_x - start_x);//�ص�����Ŀ���  

	//ͼ���Ȩ�ںϣ�ͨ���ı�alpha�޸�DstImg��WarpImg����Ȩ�أ��ﵽ�ں�Ч��
	double alpha = 1.0;
	for (int i = 0; i < DstImg.rows; i++)
	{
		for (int j = start_x; j < end_x; j++)
		{
			for (int c = 0; c < 3; c++)
			{
				//���ͼ��WarpImg����Ϊ0������ȫ����DstImg
				if (WarpImg.at<Vec3b>(i, j)[c] == 0)
				{
					alpha = 1.0;
				}
				else
				{
					double l = Width - (j - start_x); //�ص�������ĳһ���ص㵽ƴ�ӷ�ľ���
					alpha = l / Width;
				}
				DstImg.at<Vec3b>(i, j)[c] = DstImg.at<Vec3b>(i, j)[c] * alpha + WarpImg.at<Vec3b>(i, j)[c] * (1.0 - alpha);
			}
		}
	}

	return true;
}

//ȥ�ڱ�
Mat RemoveBlackBorder(Mat& iplImg)
{
	int width = iplImg.size().width;
	int height = iplImg.size().height;
	int d = 0;
	int i = 0, j = 0;


	if (iplImg.channels() == 3)	//��ɫͼƬ
	{

		//������ɫ�߿���
		for (i = width - 1; i >= 0; i--)
		{
			bool flag = false;
			for (j = 0; j < height; j++)
			{
				int tmpb, tmpg, tmpr;
				Vec3b bgr = iplImg.at<Vec3b>(j, i);
				tmpb = bgr[0];
				tmpg = bgr[1];
				tmpr = bgr[2];
				if (tmpb <= 20 && tmpg <= 20 && tmpr <= 20)
				{
					;
				}
				else
				{
					flag = true;
					d = i;
					break;
				}
			}
			if (flag) break;
		}


		//printf("�� d: %d\n", d);
	}

	//����ͼ��
	int w = d, h = height;
	Mat dstImg = Mat(iplImg, Rect(0, 0, d, h));
	//Mat result = Mat(dstImg, (Rect(0, 10, 340, 640)));

	return dstImg;
}

//����ƴ�ӵ�λ��
pair<double, double>offset(Mat image_left, Mat image_right, bool draw)
{
	//����SIFT���������
	int Hessian = 50000;
	cv::Ptr<cv::Feature2D> feature = cv::SIFT::create(Hessian);  // ��ȡpointNum��������


	//����ͼ��������⡢��������
	vector<KeyPoint>keypoint_left, keypoint_right;
	Mat descriptor_left, descriptor_right;
	feature->detectAndCompute(image_left, Mat(), keypoint_left, descriptor_left);
	feature->detectAndCompute(image_right, Mat(), keypoint_right, descriptor_right);

	//ʹ��FLANN�㷨�������������ӵ�ƥ��
	//FlannBasedMatcher matcher;

	//ʹ�ñ���ƥ���㷨�������������ӵ�ƥ��
	BFMatcher matcher;
	vector<DMatch>matches;
	matcher.match(descriptor_left, descriptor_right, matches);


	double Max = 0.0;
	double Min = 100;
	for (int i = 0; i < matches.size(); i++)
	{
		//float distance �C>������һ��ƥ�������������������������������ŷ�Ͼ��룬��ֵԽСҲ��˵������������Խ����
		double dis = matches[i].distance;
		if (dis > Max)
		{
			Max = dis;
		}

	}

	//ɸѡ��ƥ��̶ȸߵĹؼ���
	vector<DMatch>goodmatches;
	vector<Point2f>goodkeypoint_left, goodkeypoint_right;
	for (int i = 0; i < matches.size(); i++)
	{
		double dis = matches[i].distance;
		if (dis < 0.6 * Max)
		{
			/*
			����ͼ��͸�ӱ任
			��ͼ->queryIdx:��ѯ����������ѯͼ��
			��ͼ->trainIdx:����ѯ��������Ŀ��ͼ��
			*/
			//ע����image_rightͼ����͸�ӱ任����goodkeypoint_left��ӦqueryIdx��goodkeypoint_right��ӦtrainIdx
			//int queryIdx �C>�ǲ���ͼ�����������������descriptor�����±꣬ͬʱҲ����������Ӧ�����㣨keypoint)���±ꡣ
			goodkeypoint_left.push_back(keypoint_left[matches[i].queryIdx].pt);
			//int trainIdx �C> ������ͼ������������������±꣬ͬ��Ҳ����Ӧ����������±ꡣ
			goodkeypoint_right.push_back(keypoint_right[matches[i].trainIdx].pt);
			goodmatches.push_back(matches[i]);
		}
	}

	//����������
	if (draw)
	{
		Mat result;
		drawMatches(image_left, keypoint_left, image_right, keypoint_right, goodmatches, result,
			cv::Scalar(0, 0, 255), cv::Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		/*drawMatches(image_left, keypoint_left, image_right, keypoint_right, goodmatches, result,
			cv::Scalar(0, 0, 255), Scalar(0, 0, 255),);*/
			/*namedWindow("����ƥ��", WINDOW_NORMAL);
			imshow("����ƥ��", result);*/

		Mat temp_left = image_left.clone();
		for (int i = 0; i < goodkeypoint_left.size(); i++)
		{
			circle(temp_left, goodkeypoint_left[i], 10, Scalar(0, 255, 0), 1);
		}
		/*namedWindow("goodkeypoint_left", WINDOW_NORMAL);
		imshow("goodkeypoint_left", temp_left);*/

		Mat temp_right = image_right.clone();
		for (int i = 0; i < goodkeypoint_right.size(); i++)
		{
			circle(temp_right, goodkeypoint_right[i], 10, Scalar(0, 0, 255), 1);
		}
		/*namedWindow("goodkeypoint_right", WINDOW_NORMAL);
		imshow("goodkeypoint_right", temp_right);*/
	}

	//findHomography���㵥Ӧ�Ծ���������Ҫ4����
	/*
	��������ά���֮������ŵ�ӳ��任����H��3x3����ʹ��MSE��RANSAC�������ҵ���ƽ��֮��ı任����
	*/
	//if (goodkeypoint_left.size() < 4 || goodkeypoint_right.size() < 4) return false;


	//��ȡͼ��right��ͼ��left��ͶӰӳ����󣬳ߴ�Ϊ3*3
	//ע��˳��srcPoints��Ӧgoodkeypoint_right��dstPoints��Ӧgoodkeypoint_left


	vector<unsigned char>listpoints;
	Mat H = findHomography(goodkeypoint_right, goodkeypoint_left, RANSAC, 7, listpoints);

	std::vector< DMatch > goodgood_matches;
	for (int i = 0; i < listpoints.size(); i++)
	{
		if ((int)listpoints[i])
		{

			goodgood_matches.push_back(goodmatches[i]);
		}

	}

	Mat copysrcImage1 = image_left.clone();
	Mat copysrcImage2 = image_right.clone();
	Mat Homgimg_matches;
	drawMatches(copysrcImage1, keypoint_left, copysrcImage2, keypoint_right,
		goodgood_matches, Homgimg_matches, Scalar::all(-1), Scalar::all(-1),
		vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);



	/*cout << "goodgood_matches��С��" << goodgood_matches.size() << endl;
	cout << "good_matches��С��" << goodmatches.size() << endl;*/
	imshow("ȥ����ƥ����;", Homgimg_matches);

	//waitKey(2000);


	//͸�ӱ任���Ͻ�(0,0,1)
	Mat V2 = (Mat_<double>(3, 1) << 0.0, 0.0, 1.0);
	Mat V1 = H * V2;
	Point left_top;
	left_top.x = V1.at<double>(0, 0) / V1.at<double>(2, 0);
	left_top.y = V1.at<double>(0, 1) / V1.at<double>(2, 0);
	if (left_top.x < 0)left_top.x = 0;

	//͸�ӱ任���½�(0,src.rows,1)
	V2 = (Mat_<double>(3, 1) << 0.0, image_left.rows, 1.0);
	V1 = H * V2;
	Point left_bottom;
	left_bottom.x = V1.at<double>(0, 0) / V1.at<double>(2, 0);
	left_bottom.y = V1.at<double>(0, 1) / V1.at<double>(2, 0);
	if (left_bottom.x < 0)left_bottom.x = 0;

	int start_x = min(left_top.x, left_bottom.x);//�غ��������
	int end_x = image_left.cols;//�غ������յ�

	/*cout <<"left_bottom.y:" << left_bottom.y << endl;
	cout <<"left_top.y:" << left_top.y << endl;*/
	double verticalOffset;
	cout << "verticalOffsetԭʼ" << H.at<double>(1, 2) << endl;
	if (H.at<double>(1, 2) < 0)
	{
		verticalOffset = -(H.at<double>(1, 2)) + 1;
	}
	else
	{
		verticalOffset = -(H.at<double>(1, 2)) - 1;
	}


	// �������ƫ��
	std::cout << "Vertical offset: " << verticalOffset << std::endl;


	return make_pair(start_x, verticalOffset);//������ʼλ�ú����µ�ƫ��

}





//������������ͼ�����µ�ƫ��
Point offsety(Mat imgsrc1, Mat imgsrc2)
{
	Mat image1, image2;
	if (imgsrc1.cols >= imgsrc2.cols)
	{
		Mat img2;
		copyMakeBorder(imgsrc2, img2, 0, 0, 0, imgsrc1.cols - imgsrc2.cols, BORDER_CONSTANT, (0, 0, 0));
		image1 = imgsrc1;
		image2 = img2;
	}
	else
	{
		Mat img1;
		copyMakeBorder(imgsrc1, img1, 0, 0, 0, imgsrc2.cols - imgsrc1.cols, BORDER_CONSTANT, (0, 0, 0));
		image1 = img1;
		image2 = imgsrc2;
	}


	/*ת�Ҷ�ͼ��*/
	Mat image1_gray, image2_gray;
	cvtColor(image1, image1_gray, COLOR_BGR2GRAY);
	cvtColor(image2, image2_gray, COLOR_BGR2GRAY);

	//��λ��ؼ���ƫ����
	Mat grayL64F, grayR64F;
	image1_gray.convertTo(grayL64F, CV_64F);
	image2_gray.convertTo(grayR64F, CV_64F);
	Point shiftPt = phaseCorrelate(grayL64F, grayR64F);
	return shiftPt;
}





Mat Image_Stitching(Mat imgsrc1, Mat imgsrc2, double image_x, double image_y, bool flag)
{


	Mat image1, image2;
	if (imgsrc1.cols >= imgsrc2.cols)
	{
		/*Mat img2(imgsrc2.rows, imgsrc1.cols, imgsrc1.type());
		Mat roright = img2(Rect(0, 0, imgsrc2.cols, imgsrc2.rows));
		imgsrc2.copyTo(roright);*/
		Mat img2;
		copyMakeBorder(imgsrc2, img2, 0, 0, 0, imgsrc1.cols - imgsrc2.cols, BORDER_CONSTANT, (0, 0, 0));
		image1 = imgsrc1;
		image2 = img2;
	}
	else
	{
		/*Mat img1(imgsrc1.rows, imgsrc2.cols, imgsrc2.type());
		Mat roleft = img1(Rect(0, 0, imgsrc1.cols, imgsrc1.rows));
		imgsrc1.copyTo(roleft);*/

		Mat img1;
		copyMakeBorder(imgsrc1, img1, 0, 0, 0, imgsrc2.cols - imgsrc1.cols, BORDER_CONSTANT, (0, 0, 0));
		image1 = img1;
		image2 = imgsrc2;
	}

	imshow("zuo", image1);
	imshow("you", image2);


	/*ת�Ҷ�ͼ��*/
	Mat image1_gray, image2_gray;
	cvtColor(image1, image1_gray, COLOR_BGR2GRAY);
	cvtColor(image2, image2_gray, COLOR_BGR2GRAY);

	//��λ��ؼ���ƫ����
	Mat grayL64F, grayR64F;
	image1_gray.convertTo(grayL64F, CV_64F);
	image2_gray.convertTo(grayR64F, CV_64F);
	Point shiftPt = phaseCorrelate(grayL64F, grayR64F);
	cout << "shiftPt" << shiftPt << endl;
	//int img_y;// = shiftPt.y;
	//if (shiftPt.x > 50||shiftPt.x<-30)
	//{
	//	img_y = shiftPt.y;//���µ�ƫ����
	//}
	//else
	//{
	//	img_y = image_y;
	//}

	//ģ��ƥ�����ƴ�Ӵ�λ��
	int y1 = 50;
	int y2 = 300;
	int x1 = 1;
	int x2 = 50;
	Mat temp = image2_gray(Range(y1, y2), Range(x1, x2));
	//Mat temp = image2_gray(Range(50, 300), Range(1, 50));
	/*�������ͼ��,��С����������*/
	//Mat res(image1_gray.rows - temp.rows + 1, image2_gray.cols - temp.cols + 1, CV_32FC1);
	Mat res(image1_gray.cols - temp.cols + 1, image2_gray.rows - temp.rows + 1, CV_32FC1);
	/*ģ��ƥ�䣬���ù�һ�����ϵ��ƥ��*/
	matchTemplate(image1_gray, temp, res, TM_CCOEFF_NORMED);
	/*cout << image1_gray.cols - temp.cols + 1 << endl;
	cout << image2_gray.rows - temp.rows + 1 << endl;*/
	/*���������ֵ������*/
	threshold(res, res, 0.7, 1, THRESH_TOZERO);
	//imshow("res", res);


	double minVal, maxVal, threshold = 0.7;
	/*�������ֵ��λ��*/
	Point minLoc, maxLoc;
	minMaxLoc(res, &minVal, &maxVal, &minLoc, &maxLoc);

	cout << maxVal << endl;
	cout << "minLoc.x" << minLoc.x << endl;
	cout << "maxLoc.x" << maxLoc.x << endl;
	cout << "minLoc.y" << minLoc.y << endl;
	cout << "maxLoc.y" << maxLoc.y << endl;


	//int img_y = image_y;
	int img_y;
	image1 = imgsrc1;
	image2 = imgsrc2;

	int x;
	if (maxVal == 0)
	{
		x = image_x;
		//x= image_x;//ͼ��ƴ�ӵ�λ��
		//img_y = y1 - maxLoc.y;
		img_y = shiftPt.y;
	}
	else
	{
		x = maxLoc.x;
		img_y = y1 - maxLoc.y;
	}

	cout << "ͼ�������ƫ��Ϊ��" << img_y << endl;
	cout << "ͼ���ƴ�Ӵ�Ϊ��" << x << endl;
	/*ͼ��ƴ��*/
	Mat image_left;
	Mat image_right;


	////����������������ƫ�ƶ���ͼ�����и��ƫ����
	//if (shiftPt.y < 0)
	//{
	//	//Rect m_rect_left = Rect(0, 0, temp1.cols, temp1.rows);
	//	Rect m_rect_left = Rect(0, img_y, temp1.cols, temp1.rows - img_y);
	//	image_left = temp1(m_rect_left);
	//	image_right = image2;
	//	imshow("image_left", image_left);

	//}
	////��ͼ���»�ƫ�Ƶ����
	//else
	//{
	//	image_left = temp1;
	//	//Rect m_rect_right = Rect(0, img_y+3, image2.cols, image2.rows - img_y-3);
	//	Rect m_rect_right = Rect(0, img_y, image2.cols, image2.rows - img_y);
	//	image_right = image2(m_rect_right);
	//	imshow("image_right", image_right);
	//}
	//image_left.copyTo(Mat(result, Rect(0, 0, maxLoc.x, image_left.rows)));
	//image_right.copyTo(Mat(result, Rect(maxLoc.x - 1, 0, image_right.cols, image_right.rows)));
	//

	//result:ƴ�Ӻ��ͼ�񣬸����ģ�����ƫ����
	//temp1:ԭͼ1�ķ�ģ�岿��

	//�൱��͸�ӱ任�Ľ��
	Mat result1 = Mat::zeros(cvSize(image1.cols + image2.cols, image1.rows), image1.type());

	//result = Mat::zeros(cvSize(x + image2.cols, image1.rows), image1.type());

	//����������������ƫ��(����ͼΪ��׼��ֻ����ͼ���в���)
	if (img_y < 0)
	{
		img_y = abs(img_y);
		Rect m_rect_left = Rect(0, img_y, image1.cols, image1.rows - img_y);
		image_left = image1(m_rect_left);
		image_right = image2;
		image_right.copyTo(Mat(result1, Rect(x, 0, image_right.cols, image_right.rows)));
		imshow("result1", result1);
		Mat result = result1.clone();
		image_left.copyTo(Mat(result, Rect(0, 0, image_left.cols, image_left.rows)));

		OptimizeSeam(x, image1.cols, result1, result);

		namedWindow("name", WINDOW_NORMAL);
		imshow("name", result);
		return result;
	}
	//��ͼ���»�ƫ�Ƶ����
	else
	{
		//Rect m_rect_right = Rect(0, img_y, image2.cols, image2.rows - img_y);
		img_y = abs(img_y);
		image_left = image1;
		//Rect m_rect_right = Rect(0, img_y+3, image2.cols, image2.rows - img_y-3);
		Rect m_rect_right = Rect(0, img_y, image2.cols, image2.rows - img_y);
		image_right = image2(m_rect_right);
		image_right.copyTo(Mat(result1, Rect(x, 0, image_right.cols, image_right.rows)));
		imshow("result1", result1);
		Mat result = result1.clone();
		image_left.copyTo(Mat(result, Rect(0, 0, image_left.cols, image_left.rows)));
		OptimizeSeam(x, image1.cols, result1, result);
		namedWindow("name", WINDOW_NORMAL);
		imshow("name", result);
		return result;
	}




	//cout << "result_y" << result.rows << endl;
	/*image_left.copyTo(Mat(result, Rect(0, 0, x, image_left.rows)));
	image_right.copyTo(Mat(result, Rect(x - 1, 0, image_right.cols, image_right.rows)));*/

	//OptimizeSeam(image_left, image_right, result, x-20);
	namedWindow("name", WINDOW_NORMAL);
	//imshow("name", result);
	//return result;

}



int main()
{


	int index;
	cout << "�ڼ���ͼƬ��ʼƴ��" << endl;
	cin >> index;
	stringstream stream;
	string str;
	stream << index;
	stream >> str;
	//��һ��ͼƬ
	//string firstname = "��ǿ//" + str1 + ".jpg";
	//string firstname = "ͼƬ//" + str1 + ".jpg";
	//string firstname = "E://������������Ƭ//����7//ˮƽУ����ü���ǿ//" + str + ".jpg";
	//string firstname = "D://������������Ƭ//�ü�����//�ü���//��бУ������ǿ����//" + str1 + ".jpg";
	//string firstname = "E://������������Ƭ//����8//ˮƽУ����ü���ǿ//�ü�һ//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//ƫ������//У��//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//������ĸ//ˮƽУ��//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//����ʮ��//ƴ�ӽ��//" + str1 + ".jpg";
	//string firstname = "E://������������Ƭ//����ֱ//У��//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//������//У��//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//����ʮ��//У��//" + str + ".jpg";
	string firstname = "G://������������Ƭ//43//��бУ����ǿ//" + str + ".jpg";
	//string firstname = "E://������������Ƭ//�⻬���//�ü��ױ�//" + str + ".jpg";
	Mat left = imread(firstname);

	/*Mat img_left= Mat::zeros(cvSize(left.cols, 570),left.type());
	left.copyTo(Mat(img_left, Rect(0, 50, left.cols, left.rows)));*/
	//Mat base = left;
	//���ͼƬ
	Mat  stitchedImage;


	int n;
	cout << "��������ƴ�ӵ�ͼƬ����������1С��18��" << endl;
	cin >> n;
	cout << "����ɹ�����ʼ��ʱ" << endl;

	double image_y = 0;//���һ��ͼƬ����ͼƬ������ƫ��
	for (int k = index + 1; k <= index + n - 1; k++)
	{
		stringstream stream1;
		string str1;
		stream1 << k - 1;
		stream1 >> str1;
		//string filename = "E://������������Ƭ//ƫ������//У��//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//����ֱ//У��//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//����7//ˮƽУ����ü���ǿ//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//������ĸ//ˮƽУ��//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//�⻬���//�ü��ױ�//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//����8//ˮƽУ����ü���ǿ//�ü�һ//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//������//У��//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//����ֱ//У��//" + str1 + ".jpg";
		//string filename = "E://������������Ƭ//����ʮ��//У��//" + str1 + ".jpg";
		string filename = "G://������������Ƭ//43//��бУ����ǿ//" + str1 + ".jpg";


		stringstream stream2;
		string str2;
		stream2 << k;
		stream2 >> str2;
		//string filename1 = "E://������������Ƭ//ƫ������//У��//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//������//У��//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//������ĸ//ˮƽУ��//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//�⻬���//�ü��ױ�//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//����8//ˮƽУ����ü���ǿ//�ü�һ//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//����ֱ//У��//" + str2 + ".jpg";
		//string filename1 = "E://������������Ƭ//����ʮ��//У��//" + str2 + ".jpg";
		string filename1 = "G://������������Ƭ//43//��бУ����ǿ//" + str2 + ".jpg";
		cout << "����ƴ��......." << filename1 << endl;
		Mat right = imread(filename1);
		/*	Mat img_right = Mat::zeros(cvSize(right.cols, 570), right.type());
			right.copyTo(Mat(img_right, Rect(0, 50, right.cols, right.rows)));*/
		if (left.empty() || right.empty())
		{
			cout << "No Image!" << endl;
			system("pause");
			return -1;
		}

		//����ƴ�Ӵ���λ��
		Mat index = imread(filename);
		Point p = offsety(index, right);
		double image_x = 0;//ƴ�Ӵ���λ��
		if (p.x < -30)
		{
			image_x = left.cols - index.cols + abs(p.x);
		}
		else
		{
			image_x = left.cols - (right.cols - offset(index, right, true).first);//����ͼ���ƴ�Ӵ�
		}
		//cout << "offset.x"<<offset(index, right, true).first << endl;
		cout << "p:" << p << endl;
		double image_y1 = 0;
		bool flag = false;
		if (p.x < -30 || p.x>30)
		{
			image_y1 = p.y; //��������ͼ�������ƫ��
			flag = true;
		}
		else
		{
			image_y1 = 0;
		}

		image_y += image_y1;
		//image_y = 2;
		cout << "image_y1: " << image_y1 << endl;
		cout << "image_y: " << image_y << endl;
		cout << "image_x:" << image_x << endl;

		//light(base, right);

		stitchedImage = Image_Stitching(left, right, image_x, image_y, flag);
		//stitchedImage = Image_Stitching(left, right, image_x);
		stitchedImage = RemoveBlackBorder(stitchedImage);
		//img_left = stitchedImage;
		left = stitchedImage;

	}

	cout << "ƴ�ӳɹ�" << endl;
	stitchedImage = RemoveBlackBorder(stitchedImage);
	namedWindow("ResultImage", WINDOW_NORMAL);
	stitchedImage = RemoveBlackBorder(stitchedImage);
	Mat blur;
	GaussianBlur(stitchedImage, blur, Size(0, 0), 5);
	//USM����ǿ�㷨
	addWeighted(stitchedImage, 1.5, blur, -0.5, 0, stitchedImage);
	imshow("ResultImage", stitchedImage);


	stringstream stream3;
	string str3;
	stream3 << index;
	stream3 >> str3;
	//�����ļ���
	////string resultname = "E://������������Ƭ//����7//ƴ�ӽ��//" + str3 + ".jpg";
	////string resultname = "E://������������Ƭ//ƫ������//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//����ֱ//ƴ�ӽ��//" + str3 + ".jpg";
	//string firstname = "E://������������Ƭ//������//У��//" + str + ".jpg";
	//�����ļ���
	//string resultname = "E://������������Ƭ//������//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//����8//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//ƫ������//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//������ĸ//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//����ֱ//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//����ʮ��//ƴ�ӽ��//" + str3 + ".jpg";
	//string resultname = "E://������������Ƭ//�⻬���//ƴ�ӽ��//" + str3 + ".jpg";
	string resultname = "G://������������Ƭ//43//ƴ�ӽ��//" + str3 + ".jpg";
	imwrite(resultname, stitchedImage);
	waitKey();
	return 0;



}

