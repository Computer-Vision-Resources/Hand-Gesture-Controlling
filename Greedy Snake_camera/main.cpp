#include <iostream>
#include <Windows.h>
#include <cv.h>
#include <cxcore.h>
#include <cvaux.h>
#include <ml.h>
#include <highgui.h>
#include <string.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <time.h>

#define mpr make_pair 

using namespace std;
typedef pair<int, int> pii;

template <class T> inline T max1(T a, T b, T c = -1234567, T d = -1234567) {
	T ans =  a > b ? a : b;
	if(c != -1234567) ans = max1(ans, c);
	if(d != -1234567) ans = max1(ans, d);
	return ans;
}
template <class T> inline T min1(T a, T b, T c = -1234567, T d = -1234567) {
	T ans = a < b ? a : b;
	if(c != -1234567) ans = min(ans, c);
	if(d != -1234567) ans = min(ans, d);
	return ans;
}

const CvSize winSize = cvSize(1000, 800);
IplImage *img1, *img2;
CvCapture *pCapture = NULL;

const int step[4][2] = {{1, 0}, {-1, 0}, {0, -1}, {0, 1}};
const double maxdis = 30.0;
inline bool jiao(pii range1, pii range2){
	return range1.first <= range2.second && range2.first <= range1.second;
}
inline double dis(pii a1, pii a2){
	return sqrt(double(a1.first - a2.first) * (a1.first - a2.first) + (a1.second - a2.second) * (a1.second - a2.second));
}
inline int dir(pii a1, pii a2){
	double x = a2.first - a1.first, y = a2.second - a1.second;
	if(abs(x) < 1e-10){
		if(y > 0) return 2;
		if(y < 0) return 3;
		return -1;
	}
	if(abs(y) < 1e-10){
		if(x > 0) return 0;
		return 1;
	}
	if(x > 0 && y > 0 && abs(x) > abs(y)) return 0;
	else if(x > 0 && y > 0 && abs(x) < abs(y)) return 2;
	else if(x > 0 && y < 0 && abs(x) > abs(y)) return 0;
	else if(x > 0 && y < 0 && abs(x) < abs(y)) return 3;
	else if(x < 0 && y > 0 && abs(x) > abs(y)) return 1;
	else if(x < 0 && y > 0 && abs(x) < abs(y)) return 2;
	else if(x < 0 && y < 0 && abs(x) > abs(y)) return 1;
	else if(x < 0 && y < 0 && abs(x) < abs(y)) return 3;
	else return -1;
}

const int llast = 20;
struct segment{
	int object, r;
	pii duan;
	pii q[llast];
	segment() {
		for(int i = 0; i < llast; i++){
			q[i].first = 100000;
			q[i].second = -100000;
		}
	}
};

const int maxnoisel = 15;
const int minp = 6;
vector <pii> noise[2];
vector <int> pnoise[2];
inline bool isnoise(pii co, int i, int cate){
	if(co.first >= noise[cate][i].first - maxnoisel && co.first <= noise[cate][i].first + maxnoisel 
	&& co.second >= noise[cate][i].second - maxnoisel && co.second <= noise[cate][i].second + maxnoisel) {
		return 1;
	}
	return 0;
}

vector <pair <int, pii> > getgesture(IplImage *img, int &cate){
	const int minl = 7;
	const int maxgap = 7;

	assert(img->nChannels == 1);
	int height = img->height;
	int width = img->width;
	segment branch[1000];
	int lbranch = 0;

	int nobject = 0;
	pii object[1000];
	int sobject[1000];
	memset(sobject, 0, sizeof(sobject));
	for(int i = 0; i < 1000; i++) object[i].first = 1000000, object[i].second = -1000000;
	segment seg[2][2000];
	int lseg[2] = {0};
	pii ran[2000]; int lran = 0;

	for(int i = height - 1; i >= 0; i--){
		int now = i & 1, last = 1 - now, link = 0, lhave = 0;
		lran = 0;
		lseg[now] = 0;
		pii ranpi = mpr(0, 0);
		for (int j = 0; j < width; j++){
			unsigned char *pixel = (unsigned char*)img->imageData + img->widthStep*i + j;
			if (*pixel < 10) {
				if(lhave) {
					ranpi.second = j - 1;
					seg[now][lseg[now]++].duan = ranpi;
					ranpi = mpr(0, 0);
				}
				lhave = 0;
			} else {
				if(!lhave) ranpi.first = j;
				lhave++;
			}
		}

		// abandon lower than thershold
		if(lseg[now]){
			ran[0] = seg[now][0].duan; lran++;
			for(int j = 1; j < lseg[now]; j++){
				if(ran[lran - 1].second + maxgap >= seg[now][j].duan.first) ran[lran - 1].second = seg[now][j].duan.second;
				else ran[lran++] = seg[now][j].duan;
			}
			lseg[now] = 0;
			for(int j = 0; j < lran; j++){
				if (ran[j].second - ran[j].first >= minl) seg[now][lseg[now]++].duan = ran[j];
			}
		}

		for(int i1 = 0; i1 < lseg[now]; i1++) for(int j = 0; j < llast; j++){
			seg[now][i1].q[j].first = 10000000;
			seg[now][i1].q[j].second = -10000000;
		}
		bool f = 0, f1 = 0;
		int link1 = 0, link2 = 0;
		for(; link1 != lseg[now] && link2 != lseg[last]; ){
			if(jiao(seg[now][link1].duan, seg[last][link2].duan)){
				seg[now][link1].object = seg[last][link2].object;
				for(int j = 0; j < llast - 1; j++){
					seg[now][link1].q[j + 1].first = min1(seg[now][link1].q[j + 1].first, seg[last][link2].q[j].first);
					seg[now][link1].q[j + 1].second = max1(seg[now][link1].q[j + 1].second, seg[last][link2].q[j].second);
				}
				f = 1; f1 = 1;
				if(seg[now][link1].duan.second > seg[last][link2].duan.second)link2++, f1 = 0;
				else link1++, f = 0;
			} else {
				if(seg[now][link1].duan.second < seg[last][link2].duan.first){
					if(f == 0) seg[now][link1].object = ++nobject;
					link1++;
					f = 0;
				}else{
					if(f1 == 0) {
						branch[lbranch] = seg[last][link2];
						branch[lbranch++].r = i;
					}
					link2++;
					f1 = 0;
				}
			}
		}
		if(link1 != lseg[now] && f) link1++;
		if(link2 != lseg[last] && f1) link2++;
		for (; link1 != lseg[now]; link1++) seg[now][link1].object = ++nobject;
		for (; link2 != lseg[last]; link2++){
			branch[lbranch] = seg[last][link2];
			branch[lbranch++].r = i;
		}

		for(int j = 0; j < lseg[now]; j++){
			object[seg[now][j].object].first = min1(object[seg[now][j].object].first, seg[now][j].duan.first);
			object[seg[now][j].object].second = max1(object[seg[now][j].object].second, seg[now][j].duan.second);
			sobject[seg[now][j].object] += seg[now][j].duan.second - seg[now][j].duan.first;
			seg[now][j].q[0] = seg[now][j].duan;
		}
	}

	int lbranch1 = 0;
	for (int i = 0; i < lbranch; i++) if(!(branch[i].q[llast - 1].first > 1000 || sobject[branch[i].object] < 1000)) {
		branch[lbranch1++] = branch[i];
	}
	lbranch = 0;
	for(int i = 0; i < lbranch1; i++){
		bool ff = 0;
		for (int j = 0; j < lbranch; j++) if(branch[j].q[llast - 1] == branch[i].q[llast - 1]){
			for(int k = 0; k < llast; k++){
				branch[j].q[k].first = min1(branch[j].q[k].first, branch[i].q[k].first);
				branch[j].q[k].second = max1(branch[j].q[k].second, branch[i].q[k].second);
			}
			ff = 1;
			break;
		}
		if(!ff) branch[lbranch++] = branch[i];
	}
	int stat1[1000]; vector <pair <int, pii> > ans;
	memset(stat1, 0, sizeof(stat1));
	for(int i = 0; i < lbranch; i++){
		int l1 = object[branch[i].object].second - object[branch[i].object].first;
		int l2 = 0;
		for(int j = 0; j < llast; j++) l2 = max1(branch[i].q[j].second - branch[i].q[j].first, l2);
		bool fff = 0;
		for(int j = 0; j < noise[0].size(); j++) fff |= isnoise(mpr(branch[i].r, branch[i].duan.first), j, 0);
		if(double(l2) * 4.0 < l1 && !fff){
			stat1[branch[i].object]++;
			ans.push_back(mpr(branch[i].object, mpr(branch[i].r, branch[i].duan.first)));
		}
	}
	cate = 0;
	for(int i = 1; i <= nobject; i++) cate = max1(cate, stat1[i]);

	return ans;
}

void cvThresholdOtsu(IplImage* src, IplImage* dst)  
{  
	int height=src->height;  
	int width=src->width;  

	//histogram  
	float histogram[256]={0};  
	for(int i=0;i<height;i++) {  
		unsigned char* p=(unsigned char*)src->imageData+src->widthStep*i;  
		for(int j=0;j<width;j++) {  
			histogram[*p++]++;  
		}  
	}  
	//normalize histogram  
	int size=height*width;  
	for(int i=0;i<256;i++) {  
		histogram[i]=histogram[i]/size;  
	}  

	//average pixel value  
	float avgValue=0;  
	for(int i=0;i<256;i++) {  
		avgValue+=i*histogram[i];  
	}  

	int threshold;    
	float maxVariance=0;  
	float w=0,u=0;  
	for(int i=0;i<256;i++) {  
		w+=histogram[i];  
		u+=i*histogram[i];  

		float t=avgValue*w-u;  
		float variance=t*t/(w*(1-w));  
		if(variance>maxVariance) {  
			maxVariance=variance;  
			threshold=i;  
		}  
	}  

	cvThreshold(src,dst,threshold,255,CV_THRESH_BINARY);  
}  

void cvSkinOtsu(IplImage* src, IplImage* dst)  
{  
	assert(dst->nChannels==1&& src->nChannels==3);  

	IplImage* ycrcb=cvCreateImage(cvGetSize(src),8,3);  
	IplImage* cr=cvCreateImage(cvGetSize(src),8,1);  
	cvCvtColor(src,ycrcb,CV_BGR2YCrCb);  
	cvSplit(ycrcb,0,cr,0,0);  

	cvThresholdOtsu(cr,cr);  
	cvCopyImage(cr,dst);  
	cvReleaseImage(&cr);  
	cvReleaseImage(&ycrcb);  
}

const CvSize winsize = cvSize(1000, 800);

int main(int argc, _TCHAR* argv[])
{
	int cameraIndex=0;
	if (argc==2){
		cameraIndex=argv[1][0]-'0';
	}	

	IplImage* pImgFrame = NULL; 
	IplImage* pImgSkin = NULL;
	CvCapture* pCapture = NULL;

	cvNamedWindow("cam",1);
	cvNamedWindow("cam1", 1);

	cvMoveWindow("cam", 0, 0);

	if( !(pCapture = cvCaptureFromCAM(cameraIndex))){
		fprintf(stderr, "Can not open camera.\n");
		return -2;
	}//没有摄像头

	//first frame
	int  gesture = -1, gesture1 = -1;
	pImgFrame = cvQueryFrame( pCapture );
	pImgSkin = cvCreateImage(cvSize(pImgFrame->width, pImgFrame->height), 8, 1);

	//前景检测
	cout << "请将身体坐正，将进行前景检测。" << endl << "前景检测时请勿剧烈晃动，手不要出现在摄像头视野。" << endl;
	Sleep(3000);
	cout << "正在进行前景检测";
	vector <pair <int, pii> > rnoise;
	for(int i = 0; i < 60; i++){
		if(i % 12 == 11) cout << '.';
		pImgFrame = cvQueryFrame( pCapture ); 
		cvSkinOtsu(pImgFrame, pImgSkin);
		cvShowImage("cam", pImgFrame);
		cvShowImage("cam1", pImgSkin);
		cvSaveImage("img_temp.jpg", pImgSkin);
//		pImgSkin = cvLoadImage("img_temp.jpg", 0);
		rnoise = getgesture(pImgSkin, gesture);
		for(int j = 0; j < rnoise.size(); j++) {
			int f = 0;
			for(int i = 0; i < noise[1].size(); i++) if(isnoise(rnoise[j].second, i, 1)) pnoise[1][i]++, f++;
			if(f == 0) noise[1].push_back(rnoise[j].second), pnoise[1].push_back(1);
		}
		rnoise.clear();
	}
	for(int i = 0; i < noise[1].size(); i++) if(pnoise[1][i] > minp){
		pnoise[0].push_back(pnoise[1][i]);
		noise[0].push_back(noise[1][i]);
	}
	cout << endl;
	for(int i = 0; i < noise[0].size(); i++){
		cout << noise[0][i].first << ' ' << noise[0][i].second << endl;
	}

	///////////////////////////////////////////////////
	const int rmp = 10, cmp = 10;
	const char head = '%';
	const char food = '#';
	const char gap = '.';
	const char body = '+';
	char mp[30][30];
	memset(mp, 0, sizeof(mp));
	for(int i = 0; i < rmp; i++) for(int j = 0; j < cmp; j++) mp[i][j] = gap;
	pair <int, int> snake[500];
	int lsnake = 2;

	srand(unsigned(time(NULL)));
	bool ingame = 1;
	int nxt_food = -1;
	int lastdir = 0;
	snake[0] = mpr(rmp / 2, cmp / 2); snake[1] = mpr(rmp / 2 - 1, cmp / 2); mp[rmp / 2][cmp / 2] = head; mp[rmp / 2 - 1][cmp / 2] = body;
	nxt_food = rand() % (rmp * cmp - lsnake);
	int t = 0;
	for(int i = 0; i < rmp && t < nxt_food; i++) for(int j = 0; j < cmp && t < nxt_food; j++) {
		if(mp[i][j] == body || mp[i][j] == head) continue;
		t++;
		if(t == nxt_food){
			mp[i][j] = food;
			break;
		}
	}

	while(ingame){
		pair <int, pii> q[10];
		int n1 = 0;
		for(int i = 0; i < 10; i++){
			pImgFrame = cvQueryFrame( pCapture ); 
			cvSkinOtsu(pImgFrame, pImgSkin);
			cvShowImage("cam", pImgFrame);
			cvShowImage("cam1", pImgSkin);
			cvSaveImage("img_temp.jpg", pImgSkin);
	//		pImgSkin = cvLoadImage("img_temp.jpg", 0);

			vector <pair <int, pii> > node = getgesture(pImgSkin, gesture);
			if(gesture == 1){
				q[i].first = gesture;
				q[i].second = node[0].second;
				n1++;
			}
		}
		if(n1 >= 3){
			int i = 0;
			while(!q[i].first)
				i++;
			pii lastco = q[i].second; int lastdir1 = -1; bool fff = 0;
			for(; i < 10 && !fff; i++){
				if(!q[i].first) continue;
				int randir = dir(lastco, q[i].second);
				if(randir == lastdir1){
					if(dis(q[i].second, lastco) > maxdis) {
						fff = 1;
						break;
					}
				} else {
					if(lastdir1 != -1 && dis(q[i].second, lastco) > 20.0) fff = 1;
					lastco = q[i].second, lastdir1 = randir;
				}
			}
			if(fff && (lastdir1 != (lastdir ^ 1))) lastdir = lastdir1;
		}
		for(int i = lsnake; i > 0; i--) snake[i] = snake[i - 1];
		mp[snake[0].first][snake[0].second] = body;
		snake[0].first = (snake[0].first + step[lastdir][0] + rmp) % rmp;
		snake[0].second = (snake[0].second + step[lastdir][1] + cmp) % cmp;
		mp[snake[lsnake].first][snake[lsnake].second] = gap;
		
		if(mp[snake[0].first][snake[0].second] == food){
			mp[snake[0].first][snake[0].second] = head;
			mp[snake[lsnake].first][snake[lsnake].second] = body;
			lsnake++;
			nxt_food = rand() % (rmp * cmp - lsnake);
			int t = 0;
			for(int i = 0; i < rmp && t < nxt_food; i++) for(int j = 0; j < cmp && t < nxt_food; j++) {
				if(mp[i][j] == body || mp[i][j] == head) continue;
				t++;
				if(t == nxt_food){
					mp[i][j] = food;
					break;
				}
			}
		}else if(mp[snake[0].first][snake[0].second] == body){
			cout << "GAME OVER" << endl;
			break;
		}
		mp[snake[0].first][snake[0].second] = head;
		for(int i = 0; i < 2; i++) printf("\n");
		for(int i = 0; i < rmp; i++) {
			cout << mp[i] << endl;
			//printf("%s\n", mp[i]);
		}
		for(int i = 0; i < lsnake; i++) cout << snake[i].first << ' ' << snake[i].second << ' ';
		cout << endl;

		if( cvWaitKey(3) == 27 ){
			break;
		}
	}

	scanf("%c", mp[0][0]);
	cvDestroyWindow("cam");
	cvDestroyWindow("cam1");

	cvReleaseImage(&pImgSkin);
	cvReleaseImage(&pImgFrame);
	cvReleaseCapture(&pCapture);

	return 0;
}
