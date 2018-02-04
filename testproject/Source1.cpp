#include<iostream>
#include<cstdlib>
#include<cstdio>
#include<string>
#include<cmath>
#ifdef __APPLE__
#include<OpenCL/opencl.h>
#else
#include<CL/cl.h>
#endif

#define IMG_SIZE 100
#define MAX_SRC_SIZE (0x100000)

using namespace std;

void err_check(int err, string err_code) {
	if (err != CL_SUCCESS) {
		cout << "Error: " << err_code << "(" << err << ")" << endl;
		exit(-1);
	}
}


int main()
{
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_platform;

	cl_device_id device_id = NULL;
	cl_uint ret_num_device;

	cl_context context = NULL;

	cl_command_queue command_queue = NULL;

	cl_program program = NULL;

	cl_kernel kernel = NULL;

	cl_int err;

	float input[IMG_SIZE * 3], output[IMG_SIZE * 3];

	// Create Input data
	for (int i = 0; i<3; i++) {
		for (int j = 0; j<IMG_SIZE; ++j) {

			input[(i*IMG_SIZE) + j] = (float)(j + 1);

		}

	}

	// step 1 : getting platform ID
	err = clGetPlatformIDs(1, &platform_id, &ret_num_platform);
	err_check(err, "clGetPlatformIDs");

	// step 2 : Get Device ID
	err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_device);
	err_check(err, "clGetDeviceIDs");

	// step 3 : Create Context
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
	err_check(err, "clCreateContext");

	cl_bool sup;
	size_t rsize;
	clGetDeviceInfo(device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(sup), &sup, &rsize);
	if (sup != CL_TRUE) {
		cout << "Image not Supported" << endl;
	}
	// Step 4 : Create Command Queue
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);
	err_check(err, "clCreateCommandQueue");

	// Step 5 : Reading Kernel Program

	FILE *fp;
	size_t kernel_src_size;
	char *kernel_src_std;

	fp = fopen("write_imagef2d.cl", "r");

	kernel_src_std = (char *)malloc(MAX_SRC_SIZE);
	kernel_src_size = fread(kernel_src_std, 1, MAX_SRC_SIZE, fp);

	fclose(fp);

	//  Create Image data formate
	cl_image_format img_fmt;

	img_fmt.image_channel_order = CL_RGBA;
	img_fmt.image_channel_data_type = CL_FLOAT;

	// Step 6 : Create Image Memory Object
	cl_mem image1, image2;

	size_t width, height;
	width = height = 10;// sqrt(IMG_SIZE);

	image1 = clCreateImage2D(context, CL_MEM_READ_ONLY, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image1: clCreateImage2D");

	image2 = clCreateImage2D(context, CL_MEM_READ_WRITE, &img_fmt, width, height, 0, 0, &err);
	err_check(err, "image2: clCreateImage2D");

	// Copy Data from Host to Device
	cl_event event[5];

	size_t origin[] = { 0,0,0 }; // Defines the offset in pixels in the image from where to write.
	size_t region[] = { width, height, 1 }; // Size of object to be transferred
	err = clEnqueueWriteImage(command_queue, image1, CL_TRUE, origin, region, 0, 0, input, 0, NULL, &event[0]);
	err_check(err, "clEnqueueWriteImage");
	//cout<<kernel_src_std;
	// Step 7 : Create and Build Program
	program = clCreateProgramWithSource(context, 1, (const char **)&kernel_src_std, 0, &err);
	err_check(err, "clCreateProgramWithSource");

	err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	if (err == CL_BUILD_PROGRAM_FAILURE)
		cout << "clBulidProgram Fail...." << endl;
	err_check(err, "clBuildProgram");

	// Step 8 : Create Kernel
	kernel = clCreateKernel(program, "image_copy", &err);

	// Step 9 : Set Kernel Arguments

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&image1);

	err_check(err, "Arg 1 : clSetKernelArg");

	err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&image2);
	err_check(err, "Arg 2 : clSetKernelArg");

	// Step 10 : Execute Kernel 
	size_t GWSize[] = { width, height,1 };
	err = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, GWSize, NULL, 1, event, &event[1]);

	// Step 11 : Read output Data, from Device to Host
	err = clEnqueueReadImage(command_queue, image2, CL_TRUE, origin, region, 0, 0, output, 2, event, &event[2]);

	// Print Output
	char array[3];
	array[0] = 'R';
	array[1] = 'G';
	array[2] = 'B';
	int a;
	cout << "The Image is a combination of (R,G,B) values of the three matrices\n";
	cout << "\t\tTHE IMAGE OBTAINED BY USING read_imagef function: \n";

	for (int j = 0; j<IMG_SIZE; ++j) {
		for (a = 1; a<11; a++) {
			if (j == (10 * a))cout << endl;
		}
		cout << "( " << output[j] << "," << output[IMG_SIZE + j] << "," << output[2 * IMG_SIZE + j] << ") " << "  ";

	}

	cout << "\n\n THE SAME IMAGE VALUES HAVE BEEN WRITTEN INTO (IMAGE2) FROM (IMAGE1) BY USING write_imagef FUNCTION\n";


	cl_mem image3;

	image3 = clCreateImage2D(context, CL_MEM_READ_WRITE, &img_fmt, width, height, 0, 0, &err);

	// copy Image1 to Image3
	err = clEnqueueCopyImage(command_queue, image1, image3, origin, origin, region, 1, event, &event[3]);
	err_check(err, "clEnqueueCopyImage");


	// Step 12 : Release Objects

	clReleaseMemObject(image3);
	clReleaseMemObject(image1);
	clReleaseMemObject(image2);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	free(kernel_src_std);
	int x;
	cin >> x;
	return(0);
}