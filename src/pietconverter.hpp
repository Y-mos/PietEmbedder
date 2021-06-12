#ifndef PIETCONVERTER_HPP_
#define PIETCONVERTER_HPP_

#include <vector>
#include <opencv2/opencv.hpp>

//#define OUTPUT_DEBUG_IMAGE

class PietConverter
{
private:
	class Err {};

	cv::Mat dst;
	cv::Mat dstAscii;
	cv::Mat src;

	static const char& getPietColor_ascii(int h, int b)
	{
		static char colors[][3]
			= {
				{'R','r','0'},
				{'Y','y','1'},
				{'G','g','2'},
				{'C','c','3'},
				{'B','b','4'},
				{'M','m','5'},
		};
		static char black = 'K';
		static char white = 'W';

		if (h < 0 && b < 0) { return black; }
		else if (h < 0 || b < 0) { return white; }
		return (colors[h % 6][b % 3]);
	}

	static const cv::Scalar& getPietColor(int h, int b)
	{
		static cv::Scalar colors[][3]
			= {
				{cv::Scalar(0xC0,0xC0,0xFF),cv::Scalar(0x00,0x00,0xFF),cv::Scalar(0x00,0x00,0xC0)},
				{cv::Scalar(0xC0,0xFF,0xFF),cv::Scalar(0x00,0xFF,0xFF),cv::Scalar(0x00,0xC0,0xC0)},
				{cv::Scalar(0xC0,0xFF,0xC0),cv::Scalar(0x00,0xFF,0x00),cv::Scalar(0x00,0xC0,0x00)},
				{cv::Scalar(0xFF,0xFF,0xC0),cv::Scalar(0xFF,0xFF,0x00),cv::Scalar(0xC0,0xC0,0x00)},
				{cv::Scalar(0xFF,0xC0,0xC0),cv::Scalar(0xFF,0x00,0x00),cv::Scalar(0xC0,0x00,0x00)},
				{cv::Scalar(0xFF,0xC0,0xFF),cv::Scalar(0xFF,0x00,0xFF),cv::Scalar(0xC0,0x00,0xC0)},
		};
		static cv::Scalar black = cv::Scalar(0x00, 0x00, 0x00);
		static cv::Scalar white = cv::Scalar(0xFF, 0xFF, 0xFF);

		if (h < 0 && b < 0) { return black; }
		else if (h < 0 || b < 0) { return white; }
		return colors[h % 6][b % 3];
	}
	void init()
	{
		dst = cv::Mat();
		dstAscii = cv::Mat();
		src = cv::Mat();
	}
	void copyTo(PietConverter& dst) const
	{
		//	!!Warning : Shallow copy!!
		dst.dst = this->dst;
		dst.src = this->src;
		dst.dstAscii = this->dstAscii;
	}
	bool convertToPietImage()
	{
		if (dst.empty())
		{
			std::cerr << "!Error : Input image file is not loaded" << std::endl;
			return false;
		}
		double a = 1.0;
		if (!src.empty())
		{
			double W = this->src.cols;
			double H = this->src.rows;
			double w = this->dst.cols;
			double h = this->dst.rows;
			a = std::min(W / w, H / h);
		}
		cv::Mat dst_mini;
		cv::resize(this->dst, dst_mini, cv::Size(), a, a, cv::INTER_NEAREST);

		cv::Mat hsv;
		cv::cvtColor(dst_mini, hsv, cv::COLOR_BGR2HSV);
#ifdef OUTPUT_DEBUG_IMAGE
		cv::imwrite("obj/hsv.bmp", hsv);
#endif
		std::vector<cv::Mat> hsv_ch;
		std::vector<cv::Mat> bgr_ch;
		cv::split(hsv, hsv_ch);
		cv::split(dst_mini, bgr_ch);

		dstAscii = cv::Mat::zeros(dst_mini.size(), CV_8UC1);

		//	colors
		cv::Mat brights[3];

		cv::threshold(hsv_ch[2], brights[0], 224, 1, cv::THRESH_BINARY);
		cv::threshold(hsv_ch[2], brights[1], 96, 1, cv::THRESH_BINARY);
		cv::threshold(hsv_ch[2], brights[2], 96, 1, cv::THRESH_BINARY_INV);
		brights[1] -= brights[0];

		cv::Mat cols[7];
		cv::threshold(hsv_ch[0], cols[0], 15, 1, cv::THRESH_BINARY_INV);
		cv::threshold(hsv_ch[0], cols[1], 35, 1, cv::THRESH_BINARY_INV);//45
		cv::threshold(hsv_ch[0], cols[2], 85, 1, cv::THRESH_BINARY_INV);//75
		cv::threshold(hsv_ch[0], cols[3], 105, 1, cv::THRESH_BINARY_INV);
		cv::threshold(hsv_ch[0], cols[4], 135, 1, cv::THRESH_BINARY_INV);
		cv::threshold(hsv_ch[0], cols[5], 165, 1, cv::THRESH_BINARY_INV);
		cv::threshold(hsv_ch[0], cols[6], 165, 1, cv::THRESH_BINARY);
		cols[5] -= cols[4];
		cols[4] -= cols[3];
		cols[3] -= cols[2];
		cols[2] -= cols[1];
		cols[1] -= cols[0];
		cols[0] += cols[6];

		for (int h = 0; h < 6; h++)
		{
			for (int b = 0; b < 3; b++)
			{
				cv::Mat mask;
				cv::multiply(brights[b], cols[h], mask);
				cv::Mat tmp;
				tmp = cv::Mat(dst_mini.size(), CV_8UC1, getPietColor_ascii(h, b));
				tmp.copyTo(dstAscii, mask);

			}
		}

		//	grays
		cv::Mat maxVal = cv::max(bgr_ch[0], cv::max(bgr_ch[1], bgr_ch[2]));
		cv::Mat minVal = cv::min(bgr_ch[0], cv::min(bgr_ch[1], bgr_ch[2]));

		cv::Mat blackMask, whiteMask;
		cv::Mat tmp;
		cv::threshold(hsv_ch[1], tmp, 32, 1, cv::THRESH_BINARY_INV);
		blackMask = tmp.clone();
		whiteMask = tmp.clone();

		cv::threshold(minVal, tmp, 128, 1, cv::THRESH_BINARY);
		cv::min(whiteMask, tmp, whiteMask);
		cv::threshold(minVal, tmp, 224, 1, cv::THRESH_BINARY);
		cv::max(whiteMask, tmp, whiteMask);

		cv::threshold(maxVal, tmp, 128, 1, cv::THRESH_BINARY_INV);
		cv::min(blackMask, tmp, blackMask);
		cv::threshold(maxVal, tmp, 32, 1, cv::THRESH_BINARY_INV);
		cv::max(blackMask, tmp, blackMask);

		tmp = cv::Mat(dst_mini.size(), CV_8UC1, getPietColor_ascii(-1, -1));
		tmp.copyTo(dstAscii, blackMask);
		tmp = cv::Mat(dst_mini.size(), CV_8UC1, getPietColor_ascii(-1, 1));
		tmp.copyTo(dstAscii, whiteMask);


		return true;
	}
public:
	PietConverter()
	{
		init();
	}
	PietConverter(const PietConverter& obj)
	{
		obj.copyTo(*this);
	}
	~PietConverter()
	{

	}
	bool loadDstImage(std::string iifname)
	{
		dst = cv::imread(iifname);
		if (dst.empty())
		{
			std::cerr << "!Error : The input image file \"" << iifname << "\" cannot be opened." << std::endl;
			return false;
		}
		return true;
	}

	bool loadPietSourceText(std::string itfname)
	{
		std::ifstream ifs(itfname);
		if (!ifs.is_open())
		{
			std::cerr << "!Error : Piet design file \"" << itfname << "\" cannot be opened." << std::endl;
			return false;
		}

		std::string line;
		//	checking header
		std::getline(ifs, line);
		if (line != "PIET TEXT DESCRIPTION")
		{
			std::cerr << "!Error : The file \"" << itfname << "\" is not a Piet design file." << std::endl;
			return false;
		}
		//	checking size
		std::getline(ifs, line);
		size_t pos = line.find(',');
		if (pos == std::string::npos)
		{
			std::cerr << "!Error : The size cannot be read from the Piet design file: \"" << itfname << "\"." << std::endl;
			return false;
		}
		int W = std::atoi(line.substr(0, pos).c_str());
		int H = std::atoi(line.substr(pos + 1).c_str());

		//	getting data
		src = cv::Mat(cv::Size(W, H), CV_8UC1, cv::Scalar(0));
		for (int y = 0; y < H; y++)
		{
			std::getline(ifs, line);
			unsigned char* ptr = src.ptr(y);
			for (int x = 0; x < W; x++)
			{
				*ptr = line[x];
				ptr += src.elemSize();
			}
		}

		return true;
	}

	void ascii2piet(const cv::Mat& ascii, cv::Mat& piet)
	{
		piet = cv::Mat(ascii.size(), CV_8UC3, cv::Scalar(255, 255, 255));
		cv::Mat mask, col;
		for (int h = 0; h < 6; h++)
		{
			for (int b = 0; b < 3; b++)
			{
				char c = getPietColor_ascii(h, b);
				const cv::Scalar& color = getPietColor(h, b);
				mask = (ascii == c);
				col = cv::Mat(ascii.size(), CV_8UC3, color);
				col.copyTo(piet, mask);

			}
		}

		{
			char c = getPietColor_ascii(-1, -1);
			const cv::Scalar& color = getPietColor(-1, -1);
			mask = (ascii == c);
			col = cv::Mat(ascii.size(), CV_8UC3, color);
			col.copyTo(piet, mask);
		}

		return;
	}

	void createMask2(const cv::Mat& resAscii, cv::Mat& mask)
	{
		cv::Mat pathL = (src == '<');
		cv::Mat pathR = (src == '>');
		cv::Mat pathU = (src == '^');
		cv::Mat pathD = (src == 'v');
		cv::Mat pathI = (src == 'W');
		cv::Mat cross = (src == '+');
		cv::Mat blockB = (src == 'K');

		cv::Mat blockC(src.size(), CV_8UC1, cv::Scalar(0));
		for (int h = 0; h < 6; h++)
		{
			for (int b = 0; b < 3; b++)
			{
				cv::Mat tmp = (src == getPietColor_ascii(h, b));
				cv::max(tmp, blockC, blockC);
			}
		}

		cv::Mat diffL, diffR, diffU, diffD;
		cv::Mat kL = (cv::Mat_<double>(1, 2) << -1, 1);
		cv::Mat kR = (cv::Mat_<double>(1, 2) << 1, -1);
		cv::Mat kU = (cv::Mat_<double>(2, 1) << -1, 1);
		cv::Mat kD = (cv::Mat_<double>(2, 1) << 1, -1);
		{
			cv::Mat resAsciiS;
			resAscii.convertTo(resAsciiS, CV_32FC1);
			cv::filter2D(resAsciiS, diffL, -1, kL, cv::Point(1, 0));
			cv::filter2D(resAsciiS, diffR, -1, kR, cv::Point(0, 0));
			cv::filter2D(resAsciiS, diffU, -1, kU, cv::Point(0, 1));
			cv::filter2D(resAsciiS, diffD, -1, kD, cv::Point(0, 0));
			diffL = (diffL != 0);
			diffR = (diffR != 0);
			diffU = (diffU != 0);
			diffD = (diffD != 0);
		}
		cv::Mat isB = (resAscii == 'K');

		cv::Mat tmp, ptmp, qtmp;
		//	===== horizontal path =====
		cv::Mat pathH, pathH_inv;
		cv::max(pathL, pathR, pathH);
		pathH_inv = 255 - pathH;

		cv::Mat pathMaskUD;
		cv::max(diffU, diffD, pathMaskUD);
		cv::max(pathH_inv, pathMaskUD, pathMaskUD);

		cv::Mat pathMaskU;
		tmp = (diffD == 0);
		cv::max(diffU, tmp, pathMaskU);
		cv::max(pathH_inv, pathMaskU, pathMaskU);
		tmp = cv::Mat::zeros(pathMaskU.size(), CV_8UC1);
		ptmp = tmp(cv::Rect(cv::Point(0, 0), cv::Size(tmp.cols, tmp.rows - 1)));
		qtmp = pathMaskU(cv::Rect(cv::Point(0, 1), cv::Size(tmp.cols, tmp.rows - 1)));
		qtmp.copyTo(ptmp);
		pathMaskU = tmp.clone();

		cv::Mat pathMaskD;
		tmp = (diffU == 0);
		cv::max(diffD, tmp, pathMaskD);
		cv::max(pathH_inv, pathMaskD, pathMaskD);
		tmp = cv::Mat::zeros(pathMaskD.size(), CV_8UC1);
		ptmp = tmp(cv::Rect(cv::Point(0, 1), cv::Size(tmp.cols, tmp.rows - 1)));
		qtmp = pathMaskD(cv::Rect(cv::Point(0, 0), cv::Size(tmp.cols, tmp.rows - 1)));
		qtmp.copyTo(ptmp);
		pathMaskD = tmp.clone();

		cv::Mat pathMaskH_LR;
		cv::max(diffL, diffR, pathMaskH_LR);
		pathMaskH_LR = 255 - pathMaskH_LR;
		cv::max(pathH_inv, pathMaskH_LR, pathMaskH_LR);

		cv::Mat pathMaskH_B;
		tmp = 255 - isB;
		cv::max(pathH_inv, tmp, pathMaskH_B);

		cv::Mat maskH = cv::Mat(src.size(), CV_8UC1, cv::Scalar(255));
		cv::min(pathMaskUD, maskH, maskH);
		cv::min(pathMaskU, maskH, maskH);
		cv::min(pathMaskD, maskH, maskH);
		cv::min(pathMaskH_LR, maskH, maskH);
		cv::min(pathMaskH_B, maskH, maskH);
		/*
		*/
		//	===== vertical path =====
		cv::Mat pathV, pathV_inv;
		cv::max(pathU, pathD, pathV);
		pathV_inv = 255 - pathV;

		cv::Mat pathMaskLR;
		cv::max(diffL, diffR, pathMaskLR);
		cv::max(pathV_inv, pathMaskLR, pathMaskLR);

		cv::Mat pathMaskL;
		tmp = (diffR == 0);
		cv::max(diffL, tmp, pathMaskL);
		cv::max(pathV_inv, pathMaskL, pathMaskL);
		tmp = cv::Mat::zeros(pathMaskL.size(), CV_8UC1);
		ptmp = tmp(cv::Rect(cv::Point(0, 0), cv::Size(tmp.cols - 1, tmp.rows)));
		qtmp = pathMaskL(cv::Rect(cv::Point(1, 0), cv::Size(tmp.cols - 1, tmp.rows)));
		qtmp.copyTo(ptmp);
		pathMaskL = tmp.clone();

		cv::Mat pathMaskR;
		tmp = (diffL == 0);
		cv::max(diffR, tmp, pathMaskR);
		cv::max(pathV_inv, pathMaskR, pathMaskR);
		tmp = cv::Mat::zeros(pathMaskR.size(), CV_8UC1);
		ptmp = tmp(cv::Rect(cv::Point(1, 0), cv::Size(tmp.cols - 1, tmp.rows)));
		qtmp = pathMaskR(cv::Rect(cv::Point(0, 0), cv::Size(tmp.cols - 1, tmp.rows)));
		qtmp.copyTo(ptmp);
		pathMaskR = tmp.clone();

		cv::Mat pathMaskV_UD;
		cv::max(diffU, diffD, pathMaskV_UD);
		pathMaskV_UD = 255 - pathMaskV_UD;
		cv::max(pathV_inv, pathMaskV_UD, pathMaskV_UD);

		cv::Mat pathMaskV_B;
		tmp = 255 - isB;
		cv::max(pathV_inv, tmp, pathMaskV_B);

		cv::Mat maskV = cv::Mat(src.size(), CV_8UC1, cv::Scalar(255));
		cv::min(pathMaskLR, maskV, maskV);
		cv::min(pathMaskL, maskV, maskV);
		cv::min(pathMaskR, maskV, maskV);
		cv::min(pathMaskV_UD, maskV, maskV);
		cv::min(pathMaskV_B, maskV, maskV);
		//	===== path mask =====
		cv::Mat maskP;
		cv::min(maskH, maskV, maskP);
		//  ===== crossing point mask =====
		cv::Mat MC1;
		cv::Mat KC = (cv::Mat_<double>(3, 3) << 0, 1, 0, 1, 0, 1, 0, 1, 0);
		cv::filter2D(cross, MC1, -1, KC);
		cv::Mat MC2;
		cv::min(MC1, maskP, MC2);
		cv::Mat MC3;
		cv::filter2D(MC2, MC3, -1, KC);
		cv::Mat MC4;
		tmp = (MC3 == 0);
		ptmp = (isB == 0);
		cv::min(tmp, ptmp, MC4);
		cv::Mat maskC;
		tmp = (cross == 0);
		cv::max(tmp, MC4, maskC);

		//	===== block/joint =====
		cv::Mat maskB;
		cv::filter2D(blockC, qtmp, -1, cv::Mat::ones(3, 3, CV_8UC1));
		cv::max(pathI, blockB, maskB);
		cv::max(qtmp, maskB, maskB);
		maskB = 255 - maskB;

		cv::min(maskP, maskC, mask);
		cv::min(maskB, mask, mask);

#ifdef OUTPUT_DEBUG_IMAGE
		{
			cv::Mat img;
			ascii2piet(resAscii, img);
			cv::imwrite("obj/pre000_resAscii.bmp", img);
			ascii2piet(src, img);
			cv::imwrite("obj/pre000_src.bmp", img);

			cv::imwrite("obj/pre001_pathL.bmp", pathL);
			cv::imwrite("obj/pre002_pathR.bmp", pathR);
			cv::imwrite("obj/pre003_pathU.bmp", pathU);
			cv::imwrite("obj/pre004_pathD.bmp", pathD);
			cv::imwrite("obj/pre005_pathI.bmp", pathI);
			cv::imwrite("obj/pre006_cross.bmp", cross);
			cv::imwrite("obj/pre007_blockB.bmp", blockB);
			cv::imwrite("obj/pre008_blockC.bmp", blockC);

			cv::imwrite("obj/pre009_diffL.bmp", diffL);
			cv::imwrite("obj/pre010_diffR.bmp", diffR);
			cv::imwrite("obj/pre011_diffU.bmp", diffU);
			cv::imwrite("obj/pre012_diffD.bmp", diffD);

			cv::imwrite("obj/pre013_isB.bmp", diffD);
			//
			cv::imwrite("obj/hp000_pathH.bmp", pathH);
			cv::imwrite("obj/hp001_pathMaskUD.bmp", pathMaskUD);
			cv::imwrite("obj/hp002_pathMaskU.bmp", pathMaskU);
			cv::imwrite("obj/hp003_pathMaskD.bmp", pathMaskD);
			cv::imwrite("obj/hp004_pathMaskH_LR.bmp", pathMaskH_LR);
			cv::imwrite("obj/hp005_pathMaskH_B.bmp", pathMaskH_B);
			cv::imwrite("obj/hp006_maskH.bmp", maskH);

			cv::imwrite("obj/vp000_pathV.bmp", pathV);
			cv::imwrite("obj/vp001_pathMaskLR.bmp", pathMaskLR);
			cv::imwrite("obj/vp002_pathMaskL.bmp", pathMaskL);
			cv::imwrite("obj/vp003_pathMaskR.bmp", pathMaskR);
			cv::imwrite("obj/vp004_pathMaskV_UD.bmp", pathMaskV_UD);
			cv::imwrite("obj/vp005_pathMaskV_B.bmp", pathMaskV_B);
			cv::imwrite("obj/vp006_maskV.bmp", maskV);

			cv::imwrite("obj/mp000_maskP.bmp", maskP);

			cv::imwrite("obj/mc000_cross.bmp", cross);
			cv::imwrite("obj/mc001_MC1.bmp", MC1);
			cv::imwrite("obj/mc002_MC2.bmp", MC2);
			cv::imwrite("obj/mc003_MC3.bmp", MC3);
			cv::imwrite("obj/mc004_MC4.bmp", MC4);
			cv::imwrite("obj/mc005_maskC.bmp", maskC);

			cv::imwrite("obj/mb000_maskB.bmp", maskB);

			cv::imwrite("obj/mm000_mask.bmp", mask);
		}
#endif
		return;
	}


	bool overwrap(cv::Mat& res)
	{
		if (!convertToPietImage())
		{
			return false;
		}
		cv::Mat resAscii = cv::Mat(src.size(), CV_8UC1, cv::Scalar(255));
		cv::Mat pres = resAscii(cv::Rect(cv::Point((resAscii.cols - dstAscii.cols) / 2, (resAscii.rows - dstAscii.rows) / 2), dstAscii.size()));
		dstAscii.copyTo(pres);

		cv::Mat mask;
		createMask2(resAscii, mask);

		src.copyTo(resAscii, 255 - mask);

		ascii2piet(resAscii, res);

		return true;
	}

};

#endif
