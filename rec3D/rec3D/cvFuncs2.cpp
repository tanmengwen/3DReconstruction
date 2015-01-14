#include "header.h"

// roughly smooth the glitches on the disparity map
void FixDisparity( Mat_<float> & disp, int numberOfDisparities ) 
{
	Mat_<float> disp1;
	float lastPixel = 10;
	PARAM float minDisparity = 23;
	for (int i = 0; i < disp.rows; i++)
	{
		for (int j = numberOfDisparities; j < disp.cols; j++)
		{
			if (disp(i,j) <= minDisparity) disp(i,j) = lastPixel;
			else lastPixel = disp(i,j);
		}
	}
	PARAM int an = 4;
	copyMakeBorder(disp, disp1, an,an,an,an, BORDER_REPLICATE);
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(an*2+1, an*2+1));
	morphologyEx(disp1, disp1, CV_MOP_OPEN, element);
	morphologyEx(disp1, disp1, CV_MOP_CLOSE, element);
	disp = disp1(Range(an, disp.rows-an), Range(an, disp.cols-an)).clone();
}

void CalcDisparity( Mat &imgL, Mat &imgR, Mat_<float> &disp, int nod ) 
{
	enum { STEREO_BM=0, STEREO_SGBM=1, STEREO_HH=2 };
	int alg = STEREO_SGBM;

	StereoSGBM sgbm;
	int cn = imgR.channels();

	sgbm.SADWindowSize = 3;
	sgbm.numberOfDisparities = nod;
	sgbm.preFilterCap = 63;
	sgbm.P1 = 8*cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
	sgbm.P2 = 32*cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
	sgbm.minDisparity = 0;
	sgbm.uniquenessRatio = 10;
	sgbm.speckleWindowSize = 100;
	sgbm.speckleRange = 32;
	sgbm.disp12MaxDiff = 1;
	sgbm.fullDP = alg == STEREO_HH;

	Mat dispTemp, disp8;
	sgbm(imgL, imgR, dispTemp);
	dispTemp.convertTo(disp, CV_32FC1, 1.0/16);
	disp.convertTo(disp8, CV_8U, 255.0/nod);
	imshow("origin disparity", disp8);
	//waitKey();

	FixDisparity(disp, nod);
	//disp.convertTo(disp8, CV_8U, 255.0/nod);
	//imshow("fixed disparity", disp8);
}

void ChooseKeyPointsBM( Mat_<float> &disp, int nod, int noe, int nof,
					   vector<Point2f> & ptsL, vector<Point2f> & ptsR ) 
{
	Mat_<float> dShow, dCopy, dx, dy, dEdge;
	dCopy = disp.colRange(Range(nod, disp.cols)).clone();
	normalize(dCopy, dCopy, 0, 1, NORM_MINMAX);
	cvtColor(dCopy, dShow, CV_GRAY2RGB);
	imshow("disparity", dShow);

	PARAM int sobelWinSz = 7;
	Sobel(dCopy, dx, -1, 1, 0, sobelWinSz);
	Sobel(dCopy, dy, -1, 0, 1, sobelWinSz);
	magnitude(dx, dy, dEdge);
	normalize(dEdge, dEdge, 0, 10, NORM_MINMAX);
	//imshow("edge of disparity", dEdge);
	//waitKey();

	PARAM int filterSz[] = {50,30};
	PARAM float slope[] = {4,8};
	PARAM int keepBorder = 5;
	int cnt = 0;
	double value;
	PARAM float minValue = .003;
	Point2f selPt1, selPt2;
	Mat_<float> dEdgeCopy1 = dEdge.clone();

	// find the strongest edges, assign 1 or 2 key points near it
	while (cnt < noe)
	{

		Point loc;
		minMaxLoc(dEdgeCopy1, NULL, &value, NULL, &loc);
		if (value < minValue) break;

		float dx1 = dx(loc), dy1 = dy(loc);
		if (abs(dx1) >= abs(dy1))
		{
			selPt1.y = selPt2.y = loc.y;
			selPt1.x = loc.x - (dx1 > 0 ? slope[1] : slope[0]) + nod;
			selPt2.x = loc.x + (dx1 > 0 ? slope[0] : slope[1]) + nod;
			if (selPt1.x > keepBorder+nod)
			{
				ptsL.push_back(selPt1);
				ptsR.push_back(selPt1 - Point2f(disp(selPt1), 0));
				circle(dShow, selPt1-Point2f(nod,0), 2, CV_RGB(255,0,0), 2);
				cnt++;
			}
			if (selPt2.x < disp.cols - keepBorder)
			{
				ptsL.push_back(selPt2);
				ptsR.push_back(selPt2 - Point2f(disp(selPt2), 0));
				circle(dShow, selPt2-Point2f(nod,0), 2, CV_RGB(0,255,0), 2);
				cnt++;
			}

			imshow("disparity",dShow);
			//waitKey();

			int left = min(filterSz[1], loc.x),
				top = min(filterSz[0], loc.y),
				right = min(filterSz[1], dCopy.cols-loc.x-1),
				bot = min(filterSz[0], dCopy.rows-loc.y-1);
			Mat sub = dEdgeCopy1(Range(loc.y-top, loc.y+bot+1), Range(loc.x-left, loc.x+right+1));
			sub.setTo(Scalar(0));
			//imshow("processing disparity edge", dEdgeCopy1);
			//waitKey();
		}
		else
		{
			selPt1.x = selPt2.x = loc.x+nod;
			selPt1.y = loc.y - (dy1 > 0 ? slope[1] : slope[0]);
			selPt2.y = loc.y + (dy1 > 0 ? slope[0] : slope[1]);
			if (selPt1.y > keepBorder)
			{
				ptsL.push_back(selPt1);
				ptsR.push_back(selPt1 - Point2f(disp(selPt1), 0));
				circle(dShow, selPt1-Point2f(nod,0), 2, CV_RGB(255,255,0), 2);
				cnt++;
			}
			if (selPt2.y < disp.rows-keepBorder)
			{
				ptsL.push_back(selPt2);
				ptsR.push_back(selPt2 - Point2f(disp(selPt2), 0));
				circle(dShow, selPt2-Point2f(nod,0), 2, CV_RGB(0,255,255), 2);
				cnt++;
			}

			imshow("disparity",dShow);
			//waitKey();

			int left = min(filterSz[0], loc.x),
				top = min(filterSz[1], loc.y),
				right = min(filterSz[0], dCopy.cols-loc.x-1),
				bot = min(filterSz[1], dCopy.rows-loc.y-1);
			Mat sub = dEdgeCopy1(Range(loc.y-top, loc.y+bot+1), Range(loc.x-left, loc.x+right+1));
			sub.setTo(Scalar(0));
			//imshow("processing disparity edge", dEdgeCopy1);
			//waitKey();
		}

	}

	PARAM int filterSz0 = 6;
	PARAM keepBorder = 3;
	cnt = 0;
	Mat_<float> dEdgeCopy2;// = dEdge.clone();
	GaussianBlur(dEdge, dEdgeCopy2, Size(0,0), 5);
	char str[10];

	// find the flat areas, assign 1 key point near it
	while (cnt < nof)
	{

		Point loc;
		minMaxLoc(dEdgeCopy2, &value, NULL, &loc, NULL);
		if (value == 10) break;

		loc.x += nod;
		if (loc.x > keepBorder+nod && loc.y > keepBorder &&
			loc.x < disp.cols && loc.y < disp.rows)
		{
			ptsL.push_back(loc);
			ptsR.push_back(Point2f(loc) - Point2f(disp(loc), 0));
			circle(dShow, Point2f(loc)-Point2f(nod,0), 2, CV_RGB(255,0,255), 2);
			cnt++;
			sprintf_s(str, 10, "%.1f", disp(loc));
			putText(dShow, str, Point(loc.x-nod+3, loc.y), FONT_HERSHEY_SIMPLEX, .3, CV_RGB(255,0,255));
			imshow("disparity",dShow);
		}

		loc.x -= nod;
		int filterSz1 = (10-value*3)*filterSz0;
		int left = min(filterSz1, loc.x),
			top = min(filterSz1, loc.y),
			right = min(filterSz1, dCopy.cols-loc.x-1),
			bot = min(filterSz1, dCopy.rows-loc.y-1);
		Mat sub = dEdgeCopy2(Range(loc.y-top, loc.y+bot+1), Range(loc.x-left, loc.x+right+1));
		sub.setTo(Scalar(10));
		//imshow("processing disparity flat area", dEdgeCopy2);
	}
}

void GetPairBM( Mat &imgL, Mat &imgR, vector<Point2f> &ptsL, vector<Point2f> &ptsR ) 
{
	Mat_<float> disp;
	imshow("left image", imgL);

	PARAM int numOfDisp = 80; // number of disparity, must be divisible by 16
	CalcDisparity(imgL, imgR, disp, numOfDisp);
	Mat dispSave, dispS;
	normalize(disp, dispSave, 0, 1, NORM_MINMAX);
	dispSave.convertTo(dispSave, CV_8U, 255);
	imwrite("disp.jpg", dispSave);

	PARAM int numOfEgdePt = 80, numOfFlatPt = 50;
	ChooseKeyPointsBM(disp, numOfDisp, numOfEgdePt, numOfFlatPt, ptsL, ptsR);
	waitKey();
}
