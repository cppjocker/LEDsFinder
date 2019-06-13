#pragma once

#include <opencv2/core/core.hpp>

// class for recognition LEDs on image

class LEDsFinder {
private:
	// class for keeping countour properties
	class CountorProperties {
	public:
		CountorProperties() {
			circularity = 0;
			radius = 0;

			distance2ImageCenter = 0;
			center = cv::Point(0, 0);

			quarter_idx = 0;
		}

		// keeping contour just to plot in final image
		std::vector<cv::Point> contour;

		// main characteristic to accept decision if there is a led or just some bright object
		float circularity;
		float radius;

		cv::Point2f center;

		// distance from center of object to image center. 
		// This is very important characteristic since it is expected that location of all 4 LEDs approx the same relatively to center
		float distance2ImageCenter;

		// number of analyzing quarter
		int quarter_idx;
	};


	class LEDsCluster {
	public:
		std::vector<int> countors_idxs;
	};


public:
	LEDsFinder() {}
	// main function - get image and return result - YES or NO
	bool checkLEDs(const cv::Mat& img);
	const std::vector<std::vector<cv::Point> >& getResultContours() const { return result_countours; }
protected:
	// find contours of particular quarter of image
	std::vector<CountorProperties> findQuarterLEDContours(const cv::Mat& whole_img, cv::Point begin, cv::Point imageCenterPos, int quarter_idx);
	// get numerical properties of each contour (circularity, radius, location, etc)
	std::vector<CountorProperties> getContoursProperties(const std::vector<std::vector<cv::Point> >& contours, cv::Point quarterImageBegin, cv::Point wholeImageCenter, int quarter_idx) const;
	// creating clusters of contours from different quarters
	std::vector<LEDsCluster> createClusters(const std::vector<CountorProperties>& all_countors, float cluster_thresh) const;
	// check cluster - if it is enough quarters to create this cluster, if for some quarter there is several contours - we just keep the best (on 'circularity' criteria)
	bool checkCluster(LEDsCluster& cluster, const std::vector<CountorProperties>& all_countors) const;
protected:
	// keeping result contours of 4 LEDs just for plot
	std::vector<std::vector<cv::Point> > result_countours;

};