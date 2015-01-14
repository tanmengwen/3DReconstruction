#include "header.h"


enum Algorithm {FEATURE_PT, DENSE, DENSE_SEG};
PARAM Algorithm g_algo = DENSE; // three algorithms to select corresponding points and reconstruct 3D scene

void main(int argc, char* argv[])
{
	/************************************************************************/
	/* load and resize images                                               */
	/************************************************************************/
	string folder = "D:\\YanKe\\Study\\Virtual Reality & Human-Computer Interaction\\stereoimage-7\\",
		groupname = "Baby", // Cloth,Midd,Baby,Bowling
		//filenameL = "\\view1.png", filenameR = "\\view5.png"; // L: the camera on the left
		filenameL = "\\view1s.jpg", filenameR = "\\view5s.jpg"; // L: the camera on the left

	Mat imgL = imread(folder + groupname + filenameL), 
		imgR = imread(folder + groupname + filenameR);
	if (!(imgL.data) || !(imgR.data))
	{
		cerr<<"can't load image!"<<endl;
		exit(1);
	}

	float stdWidth = 600, resizeScale = 1;
	if (imgL.cols > stdWidth * 1.2)
	{
		resizeScale = stdWidth / imgL.cols;
		Mat imgL1,imgR1;
		resize(imgL, imgL1, Size(), resizeScale, resizeScale);
		resize(imgR, imgR1, Size(), resizeScale, resizeScale);
		imgL = imgL1.clone();
		imgR = imgR1.clone();
	}

	/************************************************************************/
	/* decide which points in the left image should be chosen               */
	/* and calculate their corresponding points in the right image          */
	/************************************************************************/
	cout<<"calculating feature points..."<<endl;
	vector<Point2f> ptsL, ptsR;
	vector<vector<Point2f>> ptsL1, ptsR1;
	if (g_algo == FEATURE_PT)
	{
		if ( ! LoadPtsPairs(ptsL, ptsR, groupname+".pairs"))	{
			GetPair(imgL, imgR, ptsL, ptsR);
			SavePtsPairs(ptsL, ptsR, groupname+".pairs");	}
	}
	else if (g_algo == DENSE)
		GetPairBM(imgL, imgR, ptsL, ptsR);
	else if (g_algo == DENSE_SEG)
		GetPairSegBM(imgL, imgR, ptsL1, ptsR1);

	/************************************************************************/
	/* calculate 3D coordinates                                             */
	/************************************************************************/
	vector<Point3f> pts3D;
	float focalLenInPixel = 3740 * resizeScale,
		baselineInMM = 160;
	Point3f center3D;
	Vec3f size3D;
	float scale = .2; // scale the z coordinate so that it won't be too large spreaded
	//float imgHinMM = 400, // approximate real height of the scene in picture, useless
	//float MMperPixel = imgHinMM / imgL.rows;
	//float focalLenInMM = focalLenInPixel * MMperPixel;
	focalLenInPixel *= scale;

	cout<<"calculating 3D coordinates..."<<endl;
	StereoTo3D(ptsL, ptsR, pts3D, 
		focalLenInPixel, baselineInMM, 
		imgL, center3D, size3D);

	/************************************************************************/
	/* Delaunay triangulation                                               */
	/************************************************************************/
	cout<<"doing triangulation..."<<endl;
	size_t pairNum = ptsL.size();
	vector<Vec3i> tri;
	if (g_algo == FEATURE_PT || g_algo == DENSE)
		TriSubDiv(ptsL, imgL, tri);
	else if (g_algo == DENSE_SEG)
		TriSubDivSeg(ptsL, ptNum, imgL, tri);

	/************************************************************************/
	/* Draw 3D scene using OpenGL                                           */
	/************************************************************************/
	glutInit(&argc, argv); // must be called first in a glut program
	InitGl(); // must be called first in a glut program

	cout<<"creating 3D texture..."<<endl;
	GLuint tex;
	if (g_algo == FEATURE_PT || g_algo == DENSE)
		tex = Create3DTexture(imgL, tri, ptsL, pts3D, center3D, size3D);
	else if (g_algo == DENSE_SEG)
		tex = Create3DTexture(imgL, tri, ptsL, pts3D, center3D, size3D);
	Show(tex, center3D, size3D);
}


void SavePtsPairs( vector<Point2f> &ptsL, vector<Point2f> &ptsR, string &filename ) 
{
	ofstream os(filename.c_str());
	vector<Point2f>::iterator iterL = ptsL.begin(),
		iterR = ptsR.begin();
	os<<ptsL.size()<<endl;

	for ( ; iterL != ptsL.end(); iterL++, iterR++)
	{
		os<<iterL->x<<'\t'<<iterL->y<<"\t\t"
			<<iterR->x<<'\t'<<iterR->y<<endl;
	}
	os.close();
}

bool LoadPtsPairs( vector<Point2f> &ptsL, vector<Point2f> &ptsR, string &filename ) 
{
	ifstream is(filename.c_str());
	if (!is)
	{
		cerr<<filename<<" unable to read"<<endl;
		return false;
	}

	Point2f buf;
	int cnt;
	is>>cnt;
	for (int i = 0; i < cnt; i++)
	{
		is>>buf.x>>buf.y;
		ptsL.push_back(buf);
		is>>buf.x>>buf.y;
		ptsR.push_back(buf);
	}
	is.close();
	return true;

}
