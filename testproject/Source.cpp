#include <CL/cl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <time.h>
#include <omp.h>
#include <io.h>

using namespace std;

const int WS = 8;

int cols, rows; //ONLY OF real image(not INCREASED)
vector<pair<int, int>> ans;
vector<int> ansSum;


vector<vector<int>> rotate(vector<vector<int>> a, int window) {

	//top
	for (int i = 0; i < window / 2; i++) {
		for (int j = 0; j < cols + window * 2; j++) {
			swap(a[i][j], a[window - i - 1][j]);
		}
	}
	//bottom
	for (int i = rows + window; i < rows + window + window / 2; i++) {
		for (int j = 0; j < cols + window * 2; j++) {
			swap(a[i][j], a[rows + window * 2 - (window - ((rows + window * 2) - i)) - 1][j]);
		}
	}
	//left
	for (int i = 0; i < rows + window * 2; i++) {
		for (int j = 0; j < window / 2; j++) {
			swap(a[i][j], a[i][window - j - 1]);
		}
	}
	//right
	for (int i = 0; i < rows + window * 2; i++) {
		for (int j = cols + window; j < cols + window + window / 2; j++) {
			swap(a[i][j], a[i][cols + window * 2 - (window - ((cols + window * 2) - j)) - 1]);
		}
	}
	return a;
}

vector<vector<int>> initIncreasedImg(vector<vector<int>> a, int window) {
	//top
	for (int i = 0; i < window; i++) {
		for (int j = window; j < cols + window; j++) {
			a[i][j] = a[i + window][j];
		}
	}
	//bottom
	for (int i = rows + window; i < rows + window * 2; i++) {
		for (int j = window; j < cols + window; j++) {
			a[i][j] = a[i - window][j];
		}
	}
	//left
	for (int i = 0; i < rows + window * 2; i++) {
		for (int j = 0; j < window; j++) {
			a[i][j] = a[i][j + window];
		}
	}
	//right
	for (int i = 0; i < rows + window * 2; i++) {
		for (int j = cols + window; j < cols + window * 2; j++) {
			a[i][j] = a[i][j - window];
		}
	}
	//mirroring top corner
	a = rotate(a, window);
	return a;
}

enum state {
	P5, DIM, MAXVAL, NUMS
};

vector<vector<int>>  readFile(const string& name) {
	int w, h, mx, t;
	int row, col;
	vector<int> pixs;
	ifstream infile(name);

	string s;
	state st = P5;

	while (getline(infile, s)) {
		if (s[0] == '#') continue;
		istringstream ss(s);
		switch (st) {
		case P5:
			st = DIM;
			break;
		case DIM:
			ss >> w >> h;
			cerr << "w:" << w << " h:" << h << endl;
			rows = h, cols = w;
			st = MAXVAL;
			break;
		case MAXVAL:
			ss >> mx;
			cerr << "mx:" << mx << endl;
			st = NUMS;
			break;
		case NUMS:
			while (ss >> t) {
				pixs.push_back(t);
			}
			break;
		}
	}

	//доводим до кратности 8
	int curWindow = 8;
	vector<vector<int>> pixels(h + curWindow * 2, vector<int>(w + curWindow * 2));
	int cnt = 0;
	for (row = curWindow; row < h + curWindow; row++) {
		for (col = curWindow; col < w + curWindow; col++) {
			pixels[row][col] = pixs[cnt++];
		}
	}
	vector<vector<int>> tmp = initIncreasedImg(pixels, curWindow);
	
	int hMulti = (h % WS == 0) ? 0 : WS - (h % WS);
	int wMulti = (w % WS == 0) ? 0 : WS - (w % WS);
	if (h % WS != 0 || w % WS != 0) {
		printf("Image was increased due to multiplicity to WS=%d\nCurrent h:%d, w:%d\n\n", WS, h + hMulti, w + wMulti);
	}
	h += hMulti;
	w += wMulti;
	rows = h;
	cols = w;
	

	vector<vector<int>> pixels1(h + (curWindow * 2), vector<int>(w + (curWindow * 2)));
	for (row = curWindow; row < h + curWindow; row++) {
		for (col = curWindow; col < w + curWindow; col++) {
			pixels1[row][col] = tmp[row][col];
		}
	}

	//доводим до кратности 8

	//int curWindow = 8;
	//vector<vector<int>> pixels(h + curWindow * 2, vector<int>(w + curWindow * 2));
	//int cnt = 0;
	//for (row = curWindow; row < h + curWindow; row++) {
	//	for (col = curWindow; col < w + curWindow; col++) {
	//		pixels[row][col] = pixs[cnt++];
	//	}
	//}
	return initIncreasedImg(pixels1, curWindow);
}

int RAE(int block_i, int block_j, int cur_i, int cur_j, vector<vector<int>> &a, vector<vector<int>> &b) {
	int sum = 0;
	for (int i = 0; i < WS; i++) {
		for (int j = 0; j < WS; j++) {
			sum += (a[block_i + i][block_j + j] - b[cur_i + i][cur_j + j]) * (a[block_i + i][block_j + j] - b[cur_i + i][cur_j + j]);
		}
	}
	return sum;
}

void motionComp(vector<vector<int>> &a, vector<vector<int>> &b) {
	for (int block_i = WS; block_i <= rows; block_i += WS) {
		for (int block_j = WS; block_j <= cols; block_j += WS) {
			int min = 1000000000;
			pair<int, int> offset = make_pair(0, 0);
			//#pragma omp parallel for ordered
			for (int cur_i = block_i - WS; cur_i <= block_i + WS; cur_i++) {
				for (int cur_j = block_j - WS; cur_j <= block_j + WS; cur_j++) {
					int tmp = RAE(block_i, block_j, cur_i, cur_j, a, b);
					//#pragma omp ordered
					if (tmp < min) {
						min = tmp;
						offset = make_pair(cur_i - block_i, cur_j - block_j);
					}
				}
			}
			ans.push_back(offset);
			ansSum.push_back(min);
		}
	}
}

void err_check(int err, string err_code) {
	if (err != CL_SUCCESS) {
		cout << "Error: " << err_code << "(" << err << ")" << endl;
		exit(-1);
	}
}

void parallelMotionComp(float* a, float* b) {
	cl_uint num_entries = 100;
	cl_platform_id platforms[100];
	cl_uint num_platforms = 0;
	cl_int fn = clGetPlatformIDs(num_entries, platforms, &num_platforms);
	printf("\n\nNumber of platforms = %d\n", num_platforms);

	cl_device_type device_type = CL_DEVICE_TYPE_GPU;
	cl_device_id devices[10];
	cl_uint num_devices = 0;

	cl_int n1 = clGetDeviceIDs(platforms[1], device_type, num_entries, devices, &num_devices);
	printf("Number of devices = %d\n", num_devices);

	char deviceNames[10240];
	printf("------------------------------------------\n");
	for (int i = 0; i < num_devices; i++) {
		cl_int aa = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(deviceNames), deviceNames, NULL);
		printf("%s\n", deviceNames);
	}
	printf("------------------------------------------\n\n");

	cl_int error = 0;
	cl_context context = clCreateContext(NULL, num_devices, &devices[0], NULL, NULL, &error);
	if (error == 0) {
		printf("Context status: OK\n");
	} else {
		printf("Context status: ERROR\n");
	}

	cl_command_queue com_queue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, &error);
	if (error == 0) {
		printf("Command queue status: OK\n");
	} else {
		printf("Command queue status: ERROR\n");
	}


	FILE * pFile;
	size_t lSize;
	char * buff;
	size_t lengths;

	pFile = fopen("clfile.cl", "rb");
	if (pFile == NULL) {
		fputs("File error", stderr);
		exit(1);
	}

	lSize = _filelength(fileno(pFile));
	buff = (char*)malloc(sizeof(char)*lSize);
	if (buff == NULL) {
		fputs("Memory error", stderr);
		exit(2);
	}

	// copy the file into the buffer:
	lengths = fread(buff, 1, lSize, pFile);
	if (lengths != lSize) { fputs("Reading error", stderr); exit(3); }

	fclose(pFile);
	const char *strings = const_cast<char*>(buff);
	cl_program prog = clCreateProgramWithSource(context, 1, &strings, &lSize, &error);

	cl_int builder = clBuildProgram(prog, 1, &devices[0], "", NULL, NULL);

	char buffer[2048];
	cl_int err = clGetProgramBuildInfo(prog, devices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
	printf("%s\n", buffer);

	cl_image_format img_fmt;
	img_fmt.image_channel_order = CL_R;
	img_fmt.image_channel_data_type = CL_FLOAT;
	cl_mem image1, image2, image3;
	size_t width = (cols + WS * 2), height = rows + WS * 2;
	image1 = clCreateImage2D(context, CL_MEM_READ_ONLY, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image1: clCreateImage2D");
	image2 = clCreateImage2D(context, CL_MEM_READ_ONLY, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image2: clCreateImage2D");
	image3 = clCreateImage2D(context, CL_MEM_READ_WRITE, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image3: clCreateImage2D");


	const char *kernel_name = "motion_comp_local";
	cl_kernel ker = clCreateKernel(prog, kernel_name, &error);

	cl_mem buff_motionVec = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int2)*(rows / WS) * (cols / WS), NULL, &error);

	err = clSetKernelArg(ker, 0, sizeof(image1), &image1);
	err_check(err, "Arg 1 : clSetKernelArg");
	err = clSetKernelArg(ker, 1, sizeof(image2), &image2);
	err_check(err, "Arg 2 : clSetKernelArg");
	clSetKernelArg(ker, 2, sizeof(buff_motionVec), &buff_motionVec);
	clSetKernelArg(ker, 3, sizeof(image3), &image3);

	cl_int2* motionVec = new cl_int2[(rows / WS) * (cols / WS)];

	size_t origin[] = { 0,0,0 }; // Defines the offset in pixels in the image from where to write.
	size_t region[] = { width, height, 1 }; // Size of object to be transferred
	err = clEnqueueWriteImage(com_queue, image1, CL_TRUE, origin, region, 0, 0, a, 0, NULL, NULL);
	err = clEnqueueWriteImage(com_queue, image2, CL_TRUE, origin, region, 0, 0, b, 0, NULL, NULL);


	const size_t global_work_size[] = { cols, rows, 1};
	const size_t local_work_size[] = { WS, WS };


	cl_event timer;
	cl_int err_code = clEnqueueNDRangeKernel(com_queue, ker, 2, 0, global_work_size, local_work_size, NULL, NULL, &timer);

	

	if (err_code == 0) {
		printf("Kernel execution was successfull\n\n");
	} else {
		printf("Kernel ERROR%d\n\n", err_code);
	}

	clFinish(com_queue);
	cl_ulong start, end;
	clGetEventProfilingInfo(timer, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
	clGetEventProfilingInfo(timer, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
	printf("time: %llu\n", end - start);
	

	clEnqueueReadBuffer(com_queue, buff_motionVec, CL_TRUE, 0, sizeof(cl_int2)*(rows / WS) * (cols / WS), motionVec, NULL, NULL, NULL);

	bool check = true;
	for (int i = 0; i < (rows / WS) * (cols / WS); i++) {
		//cout << "CL:" << motionVec[i].x << ',' << motionVec[i].y << '\n';
		//cout << "C+:" << ans[i].first << ',' << ans[i].second << ' ' << ansSum[i] << endl << endl;
		//if (motionVec[i].x != ans[i].first || motionVec[i].y != ans[i].second) {
		//	check = false;
		//	cout << "i=" << i;
		//	break;
		//}
		motionVec[i].x = ans[i].first;
		motionVec[i].y = ans[i].second;
	}
	//if (check) {
	//	cout << "Hurray! Motion vectors are correct!";
	//} else {
	//	cout << "error :(";
	//}

	

	//vector<float> output(height * width);
	//clEnqueueReadImage(com_queue, image3, CL_TRUE, origin, region, 0, 0, output.data(), NULL, NULL, NULL);


	//ofstream out("outputfromdevice.pgm");
	//out << "P2\n" << width << ' ' << height << "\n255\n";
	//for (int i = 0; i < width*height; i++) {
	//	out << output[i] << '\n';
	//}
	//out.close();

	clReleaseMemObject(buff_motionVec);
	clReleaseProgram(prog);

}

int main() {


	vector<vector<int>> file1 = readFile("frame0015.pgm");
	vector<vector<int>> file2 = readFile("frame0016.pgm");
	cout << endl;


	float* a = new float[(rows + WS * 2)*(cols + WS * 2) * 3];
	float* b = new float[(rows + WS * 2)*(cols + WS * 2) * 3];

	int cnt1 = 0;
	for (int i = 0; i < rows + WS * 2; i++) {
		for (int j = 0; j < cols + WS * 2; j++) {
			a[cnt1] = (float)file1[i][j];
			b[cnt1] = (float)file2[i][j];
			cnt1++;
		}
	}

	clock_t begin_time = clock();
	motionComp(file1, file2);
	cout << "openMP compensation took:" << float(clock() - begin_time) / CLOCKS_PER_SEC;
	cout << "\nans.size:" << ans.size() << endl;

	parallelMotionComp(a, b);


	cout << endl;



	vector<vector<int>> compensated = file1;

	int cnt = 0;
	for (int x = WS; x <= rows; x += WS) {
		for (int y = WS; y <= cols; y += WS) {
			for (int i = 0; i < WS; i++) {
				for (int j = 0; j < WS; j++) {
					compensated[x + i + ans[cnt].first][y + j + ans[cnt].second] = file1[x + i][y + j];
				}
			}
			cnt++;
		}
	}

	ofstream out("result.pgm");
	out << "P2\n" << cols << ' ' << rows << "\n255\n";
	for (int i = WS; i < rows + WS; i++) {
		for (int j = WS; j < cols + WS; j++) {
			out << compensated[i][j] << '\n';
		}
	}
	out.close();

	out = ofstream("diff.pgm");
	out << "P2\n" << cols << ' ' << rows << "\n255\n";
	for (int i = WS; i < rows + WS; i++) {
		for (int j = WS; j < cols + WS; j++) {
			out << abs(file2[i][j] - compensated[i][j]) << '\n';
		}
	}
	out.close();

	system("pause");
	return 0;
}