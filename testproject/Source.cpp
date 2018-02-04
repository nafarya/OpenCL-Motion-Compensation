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

const int windowSize = 8;


int cols, rows; //ONLY OF real image(not INCREASED)
vector<pair<int, int>> ans;
vector<pair<int, int>> ans1;



vector<vector<int>> rotate(vector<vector<int>> a) {

	//top
	for (int i = 0; i < windowSize/2; i++) {
		for (int j = 0; j < cols + windowSize*2; j++) {
			swap(a[i][j], a[windowSize-i-1][j]);
		}
	}
	//bottom
	for (int i = rows + windowSize; i < rows + windowSize + windowSize/2; i++) {
		for (int j = 0; j < cols + windowSize*2; j++) {
			swap(a[i][j], a[rows+windowSize*2-(windowSize-((rows + windowSize * 2)-i)) - 1][j]);
		}
	}
	//left
	for (int i = 0; i < rows + windowSize * 2; i++) {
		for (int j = 0; j < windowSize / 2; j++) {
			swap(a[i][j], a[i][windowSize - j - 1]);
		}
	}
	//right
	for (int i = 0; i < rows + windowSize * 2; i++) {
		for (int j = cols + windowSize; j < cols + windowSize + windowSize/2; j++) {
			swap(a[i][j], a[i][cols + windowSize * 2 - (windowSize - ((cols + windowSize * 2) - j)) - 1]);
		}
	}
	return a;
}

vector<vector<int>> initIncreasedImg(vector<vector<int>> a) {
	//top
	for (int i = 0; i < windowSize; i++) {
		for (int j = windowSize - 1; j < cols+windowSize-1; j++) {
			a[i][j] = a[i + windowSize][j];
		}
	}
	//bottom
	for (int i = rows + windowSize; i < rows + windowSize * 2; i++) {
		for (int j = windowSize; j < cols + windowSize; j++) {
			a[i][j] = a[i - windowSize][j];
		}
	}
	//left
	for (int i = 0; i < rows + windowSize * 2; i++) {
		for (int j = 0; j < windowSize; j++) {
			a[i][j] = a[i][j + windowSize];
		}
	}
	//right
	for (int i = 0; i < rows + windowSize * 2; i++) {
		for (int j = cols + windowSize; j < cols + windowSize * 2; j++) {
			a[i][j] = a[i][j - windowSize];
		}
	}
	//mirroring top corner
	a = rotate(a);
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

	vector<vector<int>> pixels(h + windowSize * 2, vector<int>(w + windowSize * 2));
	int cnt = 0;
	for (row = windowSize; row < h + windowSize; row++) {
		for (col = windowSize; col < w + windowSize; col++) {
			pixels[row][col] = pixs[cnt++];
		}
	}
	return initIncreasedImg(pixels);
}

int RAE(int block_i, int block_j, int cur_i, int cur_j, vector<vector<int>> &a, vector<vector<int>> &b) {
	int sum = 0;
	for (int i = 0; i < windowSize; i++) {
		for (int j = 0; j < windowSize; j++) {
			sum += (a[block_i+i][block_j+j] - b[cur_i+i][cur_j+j]) * (a[block_i+i][block_j+j] - b[cur_i+i][cur_j+j]);
		}
	}
	return sum;
}

void motionComp(vector<vector<int>> &a, vector<vector<int>> &b) {
	for (int block_i = windowSize; block_i <= rows; block_i += windowSize) {
		for (int block_j = windowSize; block_j <= cols; block_j += windowSize) {
			int min = 1000000000;
			pair<int, int> offset = make_pair(0, 0);
//#pragma omp parallel for ordered
			for (int cur_i = block_i - windowSize; cur_i <= block_i+windowSize; cur_i++) {
				for (int cur_j = block_j - windowSize; cur_j <= block_j + windowSize; cur_j++) {		
					int tmp = RAE(block_i, block_j, cur_i, cur_j, a, b);
//#pragma omp ordered
					if (tmp < min) {
						min = tmp;
						offset = make_pair(cur_i - block_i, cur_j - block_j);
					}
				}
			}
			ans.push_back(offset);
		}
	}
}

struct Image {
	std::vector<char> pixel;
	int width, height;
};

Image LoadImage(int* imgGrayScale) {
	int width = cols+windowSize*2, height=rows+windowSize*2, maxColor=255;
	std::vector<char> data(width * height * 3);

	for (int i = 0; i < height*width; i++) {
		data[i] = imgGrayScale[i];
		data[i+width*height] = imgGrayScale[i];
		data[i+width*height] = imgGrayScale[i];
	}

	const Image img = { data, width, height };
	return img;
}

Image RGBtoRGBA(const Image& input)
{
	Image result;
	result.width = input.width;
	result.height = input.height;

	for (std::size_t i = 0; i < input.pixel.size(); i += 3) {
		result.pixel.push_back(input.pixel[i + 0]);
		result.pixel.push_back(input.pixel[i + 1]);
		result.pixel.push_back(input.pixel[i + 2]);
		result.pixel.push_back(0);
	}

	return result;
}

void SaveImage(const Image& img, const char* path)
{
	std::ofstream out(path);

	out << "P2\n";
	out << img.width << " " << img.height << "\n";
	out << "255\n";
	for (int i = 0; i < img.width*img.height; i++) {
		out << img.pixel[i] - '0' << '\n';
	}
}

Image RGBAtoRGB(const Image& input)
{
	Image result;
	result.width = input.width;
	result.height = input.height;

	for (std::size_t i = 0; i < input.pixel.size(); i += 4) {
		result.pixel.push_back(input.pixel[i + 0]);
		result.pixel.push_back(input.pixel[i + 1]);
		result.pixel.push_back(input.pixel[i + 2]);
	}

	return result;
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

	cl_device_type device_type = CL_DEVICE_TYPE_ALL;
	cl_device_id devices[10];
	cl_uint num_devices = 0;

	cl_int n1 = clGetDeviceIDs(platforms[0], device_type, num_entries, devices, &num_devices);
	printf("Number of devices = %d\n", num_devices);

	char deviceNames[200];
	printf("------------------------------------------\n");
	for (int i = 0; i < num_devices; i++) {
		cl_int aa = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 10240, deviceNames, NULL);
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


	
	const int IMG_SIZE = (rows+windowSize*2)*(cols+windowSize*2);

	cl_image_format img_fmt;
	img_fmt.image_channel_order = CL_R;
	img_fmt.image_channel_data_type = CL_FLOAT;
	cl_mem image1, image2, image3;
	size_t width= (cols + windowSize * 2), height= rows + windowSize * 2;
	image1 = clCreateImage2D(context, CL_MEM_READ_ONLY, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image1: clCreateImage2D");
	image2 = clCreateImage2D(context, CL_MEM_READ_ONLY, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image2: clCreateImage2D");
	image3 = clCreateImage2D(context, CL_MEM_READ_WRITE, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image3: clCreateImage2D");
	

	const char *kernel_name = "motion_comp";
	cl_kernel ker = clCreateKernel(prog, kernel_name, &error);

	cl_mem buff_ansX = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*(rows / windowSize) * (cols / windowSize), NULL, &error);
	cl_mem buff_ansY = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*(rows / windowSize) * (cols / windowSize), NULL, &error);

	err = clSetKernelArg(ker, 0, sizeof(image1), &image1);
	err_check(err, "Arg 1 : clSetKernelArg");
	err = clSetKernelArg(ker, 1, sizeof(image2), &image2);
	err_check(err, "Arg 2 : clSetKernelArg");

	clSetKernelArg(ker, 2, sizeof(buff_ansX), &buff_ansX);
	clSetKernelArg(ker, 3, sizeof(buff_ansY), &buff_ansY);
	clSetKernelArg(ker, 4, sizeof(int), &rows);
	clSetKernelArg(ker, 5, sizeof(int), &cols);
	clSetKernelArg(ker, 6, sizeof(image3), &image3);



	size_t origin[] = { 0,0,0 }; // Defines the offset in pixels in the image from where to write.
	size_t region[] = { width, height, 1 }; // Size of object to be transferred
	err = clEnqueueWriteImage(com_queue, image1, CL_TRUE, origin, region, 0, 0, a, 0, NULL, NULL);
	err = clEnqueueWriteImage(com_queue, image2, CL_TRUE, origin, region, 0, 0, b, 0, NULL, NULL);
	

	const size_t global_work_size[] = {cols/8, rows/8};
	const size_t local_work_size[] = {2 , 2};


	cl_event timer;
	cl_int err_code = clEnqueueNDRangeKernel(com_queue, ker, 2, NULL, global_work_size, 0, NULL, NULL, &timer);
	
	if (err_code == 0) {
		printf("Kernel execution was successfull\n\n");
	} else {
		printf("Kernel ERROR%d\n\n", err_code);
	}

	clFinish(com_queue);

	int* answerX = new int[(rows / windowSize) * (cols / windowSize)];
	int* answerY = new int[(rows / windowSize) * (cols / windowSize)];
	for (int i = 0; i < (rows / windowSize) * (cols / windowSize); i++) {
		answerX[i] = 0;
		answerY[i] = 0;
	}
	
	clEnqueueReadBuffer(com_queue, buff_ansX, CL_TRUE, 0, sizeof(int)*(rows / windowSize) * (cols / windowSize), answerX, NULL, NULL, NULL);
	clEnqueueReadBuffer(com_queue, buff_ansY, CL_TRUE, 0, sizeof(int)*(rows / windowSize) * (cols / windowSize), answerY, NULL, NULL, NULL);
	

	for (int i = 0; i < (rows / windowSize) * (cols / windowSize); i++) {
		cout << answerX[i] << ',' << answerY[i] << ' ';
	}

	vector<float> output(height * width);
	clEnqueueReadImage(com_queue, image3, CL_TRUE, origin, region, 0, 0, output.data(), NULL, NULL, NULL);
	

	ofstream out("TRY.pgm");
	out << "P2\n" << width << ' ' << height<< "\n255\n";
	for (int i = 0; i < width*height; i++) {
		out << output[i] << '\n';
	}
	out.close();

	clReleaseMemObject(buff_ansX);
	clReleaseMemObject(buff_ansY);
	clReleaseProgram(prog);


}

int main() {
	

	vector<vector<int>> file1 = readFile("myframe.pgm");
	vector<vector<int>> file2 = readFile("myframe1.pgm");
	cout << endl;


	float* a = new float[(rows + windowSize * 2)*(cols + windowSize * 2)*3];
	float* b = new float[(rows + windowSize * 2)*(cols + windowSize * 2)*3];

	int cnt1 = 0;
	for (int i = 0; i < rows + windowSize * 2; i++) {
		for (int j = 0; j < cols + windowSize * 2; j++) {
			a[cnt1] = (float)file1[i][j];
			b[cnt1] = (float)file2[i][j];
			cnt1++;
		}
	}

	clock_t begin_time = clock();
	motionComp(file1, file2);
	cout << "openMP compensation took:" << float(clock() - begin_time) / CLOCKS_PER_SEC;
	cout << "\nans.size:" << ans.size() << endl;
	for (int i = 0; i < ans.size(); i++) {
		cout << ans[i].first << ',' << ans[i].second << endl;
	}

	parallelMotionComp(a, b);

	cout << endl;
	
	

	vector<vector<int>> compensated = file1;

	int cnt = 0;
	for (int x = windowSize; x <= rows; x += windowSize) {
		for (int y = windowSize; y <= cols; y += windowSize) {
			for (int i = 0; i < windowSize; i++) {
				for (int j = 0; j < windowSize; j++) {
					compensated[x + i + ans[cnt].first][y + j + ans[cnt].second] = file1[x+i][y+j];
				}
			}
			cnt++;
		}
	}

	ofstream out("result.pgm");
	out << "P2\n" << cols << ' ' << rows << "\n255\n";
	for (int i = windowSize; i < rows + windowSize; i++) {
		for (int j = windowSize; j < cols + windowSize; j++) {
			out << compensated[i][j] << '\n';
		}
	}
	out.close();

	out = ofstream("diff.pgm");
	out << "P2\n" << cols << ' ' << rows << "\n255\n";
	for (int i = windowSize; i < rows + windowSize; i++) {
		for (int j = windowSize; j < cols + windowSize; j++) {
			out << abs(file2[i][j] - compensated[i][j]) << '\n';
		}
	}
	out.close();

	system("pause");
	return 0;
}
