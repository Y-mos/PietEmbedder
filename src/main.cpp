#include <iostream>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "pietconverter.hpp"

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cerr << "Usage : " << argv[0] << " [inputPietDesignFileName] [inputImageFileName] [outputPietCodeFileName]" << std::endl;
		return 0;
	}
	const char* itfname = argv[1];
	const char* iifname = argv[2];
	const char* ofname = argv[3];

	PietConverter pc;
	if (!pc.loadPietSourceText(itfname))
	{
		return 0;
	}
	if (!pc.loadDstImage(iifname))
	{
		return 0;
	}
	
	cv::Mat res;
	if (!pc.overwrap(res))
	{
		return 0;
	}
	cv::imshow("res", res);
	std::cerr << "Press any key on the image window..." << std::endl;
	cv::waitKey(0);

	cv::imwrite(ofname, res);

	return 0;
}