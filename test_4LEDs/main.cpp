#include <iostream>
#include <cstdlib>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#include "LEDsFinder.h"

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cout << "Usage: LEDsFinder.exe <path to image file>" << std::endl;
		return -1;
	}

	auto led_image = argv[1];


	cv::Mat image = cv::imread(led_image, CV_LOAD_IMAGE_COLOR);
	if (!image.data)
	{
		std::cerr << "Can't load image, check path: " << led_image << std::endl;
		return -1;
	}

	LEDsFinder ledsFinder;

	bool LEDs_exist = ledsFinder.checkLEDs(image);

	if (LEDs_exist) {
		std::cout << "YES" << std::endl;

		// drawing contours

		const auto& contours_for_plot = ledsFinder.getResultContours();

		cv::Mat drawing = image.clone();
		for (int i = 0; i< static_cast<int>(contours_for_plot.size()); i++)
		{
			cv::Scalar color = cv::Scalar(255, 0, 0);
			drawContours(drawing, contours_for_plot, i, color, 2, 8);
		}

		cv::imshow(led_image, drawing);
		cv::waitKey();
		cv::destroyWindow(led_image);

	}
	else {
		std::cout << "NO" << std::endl;
	}

	


	return 0;
}







