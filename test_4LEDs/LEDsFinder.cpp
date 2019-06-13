#define _USE_MATH_DEFINES

#include "LEDsFinder.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <cmath>
#include <climits>

#include <map>



bool LEDsFinder::checkLEDs(const cv::Mat& img) {
	cv::Mat img_gray;

	// convert to grayscale
	cvtColor(img, img_gray, CV_BGR2GRAY);

	// find contours for 4 quarters

	auto&& properties1 = findQuarterLEDContours(img_gray, cv::Point(0, 0), cv::Point(img.cols / 2, img.rows / 2), 0 );
	auto&& properties2 = findQuarterLEDContours(img_gray, cv::Point(img.cols / 2, 0), cv::Point(0, img.rows / 2), 1);
	auto&& properties3 = findQuarterLEDContours(img_gray, cv::Point(0, img.rows / 2), cv::Point(img.cols / 2, 0), 2 );
	auto&& properties4 = findQuarterLEDContours(img_gray, cv::Point(img.cols / 2, img.rows / 2), cv::Point(0, 0), 3);

	// aggregating in one vector
	std::vector<CountorProperties> all_contours_properties;
	all_contours_properties.insert(all_contours_properties.end(), properties1.begin(), properties1.end());
	all_contours_properties.insert(all_contours_properties.end(), properties2.begin(), properties2.end());
	all_contours_properties.insert(all_contours_properties.end(), properties3.begin(), properties3.end());
	all_contours_properties.insert(all_contours_properties.end(), properties4.begin(), properties4.end());

	// sorting by distance to center of original image
	std::sort(all_contours_properties.begin(), all_contours_properties.end(), [](const CountorProperties& a, const CountorProperties& b) {
		return a.distance2ImageCenter < b.distance2ImageCenter;
	});

	// threshold for clustering
	int thresh_clusters = std::min(img.cols, img.rows) / 7;

	// find clusters by similar distance of spot's centers to center on original image
	auto && clusters = createClusters(all_contours_properties, thresh_clusters);

	// if there was no 4 closed spots from 4 quarters - there bad LEDs 
	if (clusters.empty()) {
		return false;
	}

	for (const auto& cluster : clusters) {
		std::vector<CountorProperties> cluster_countours;

		for (auto countour_idx : cluster.countors_idxs) {
			cluster_countours.push_back(all_contours_properties[countour_idx]);
		}

		std::vector<int> circularity_good_idxs;

		int i = 0;
		for (const auto& quarter_contour : cluster_countours) {
			// we can say that each spot is good if it has good circularity
			if (quarter_contour.circularity < 2) {
				circularity_good_idxs.push_back(i);
			}
			++i;
		}

		// if all 4 spots have good circularity - we are done
		if (circularity_good_idxs.size() < 4) {
			continue;
		}


		result_countours.resize(cluster_countours.size());

		std::transform(cluster_countours.begin(), cluster_countours.end(), result_countours.begin(), [](CountorProperties& v) {
			return v.contour;
		});

		return true;

	}

	return false;
}

bool LEDsFinder::checkCluster(LEDsCluster& cluster, const std::vector<CountorProperties>& all_countors) const {
	std::map<int, std::vector<int> > quarter_stat;

	for (int cluster_idx : cluster.countors_idxs) {
		int quarter_idx = all_countors[cluster_idx].quarter_idx;
		quarter_stat[quarter_idx].push_back(cluster_idx);
	}

	// if there is no countours from all 4 quartets - we can't create cluster
	if (quarter_stat.size() < 4) {
		return false;
	}

	std::vector<int> new_cluster_idxs;

	for (const auto& stat_entry : quarter_stat) {
		const auto& quarter_idxs = stat_entry.second;
		if (quarter_idxs.size() == 1) {
			new_cluster_idxs.push_back(quarter_idxs[0]);
			continue;
		}

		// if there is several contours for some quarter - we will remian best by circularity
		int best_circularity_idx = -1;
		float best_circularity = 10000000;

		for (auto quarter_idx : quarter_idxs) {
			if (all_countors[quarter_idx].circularity < best_circularity) {
				best_circularity = all_countors[quarter_idx].circularity;
				best_circularity_idx = quarter_idx;
			}
		}

		new_cluster_idxs.push_back(best_circularity_idx);
	}

	cluster.countors_idxs = new_cluster_idxs;

	return true;
}

std::vector<LEDsFinder::LEDsCluster> LEDsFinder::createClusters(const std::vector<CountorProperties>& all_countors, float cluster_thresh) const {
	std::vector<LEDsFinder::LEDsCluster> ledsClusters;

	if (all_countors.empty()) {
		return ledsClusters;
	}

	LEDsFinder::LEDsCluster curCluster;
	curCluster.countors_idxs.push_back(0);
	float cur_distance = all_countors[0].distance2ImageCenter;

	size_t i = 1;
	while ( i < all_countors.size()) {
		if (all_countors[i].distance2ImageCenter - cur_distance > cluster_thresh) {
			if (checkCluster(curCluster, all_countors) ) {
				ledsClusters.push_back(curCluster);
			}

			curCluster.countors_idxs.clear();
		}
		else {
			curCluster.countors_idxs.push_back(i);
		}

		cur_distance = all_countors[i].distance2ImageCenter;
		++i;
	}

	if (checkCluster(curCluster, all_countors)) {
		ledsClusters.push_back(curCluster);
	}


	return ledsClusters;
}



std::vector<LEDsFinder::CountorProperties> LEDsFinder::getContoursProperties(const std::vector<std::vector<cv::Point> >& contours, cv::Point quarterImageBegin, cv::Point wholeImageCenter, int quarter_idx) const {
	std::vector<CountorProperties> contoursProperties;

	for (const auto& contour : contours) {
		// skip small countors
		if ( contour.size() < 10 ) {
			continue;
		}

		CountorProperties contourProperties;

		double next_contour_area = cv::contourArea(contour);
		double perimeter = cv::arcLength(contour, true);


		// calculate score about how much countor close to circle form
		float circularity = pow((perimeter / (2 * M_PI)), 2) * M_PI / next_contour_area;

		// skip very non-circular shapes
		if (circularity > 5) {
			continue;
		}

		int sum_x = 0;
		int sum_y = 0;


		// estimate center of circle with simple euristic 
		for (const auto& countor_point : contour) {
			sum_x += countor_point.x;
			sum_y += countor_point.y;
		}

		float center_x = static_cast<float> (sum_x) / contour.size();
		float center_y = static_cast<float> (sum_y) / contour.size();

		std::vector<float> distances2Center;
		std::vector<cv::Point> realContourCoordinates;

		for (const auto& countor_point : contour) {
			// calculate distance from each countor point to center
			distances2Center.push_back( sqrt(pow(countor_point.x - center_x, 2) + pow(countor_point.y - center_y, 2)) );
			// there we will keep coordinates corresponding to whole image
			realContourCoordinates.push_back(cv::Point(countor_point.x + quarterImageBegin.x, countor_point.y + quarterImageBegin.y));
		}

		std::sort(distances2Center.begin(), distances2Center.end());

		float radius = distances2Center[distances2Center.size() * 0.5];

		// skip small spots
		if (radius < 5) {
			continue;
		}

		cv::Point2f center(center_x + quarterImageBegin.x, center_y + quarterImageBegin.y);

		contourProperties.circularity = circularity;
		contourProperties.center = center;
		contourProperties.contour = realContourCoordinates;
		contourProperties.radius = radius;
		contourProperties.distance2ImageCenter = sqrt( pow(wholeImageCenter.x - center_x, 2) + pow(wholeImageCenter.y - center_y, 2) );
		contourProperties.quarter_idx = quarter_idx;

		contoursProperties.push_back(contourProperties);
	}

	return contoursProperties;
}

std::vector<LEDsFinder::CountorProperties> LEDsFinder::findQuarterLEDContours(const cv::Mat& whole_img, cv::Point begin, cv::Point imageCenterPos, int quarter_idx) {

	// obtain quarter of original image
	auto quarter_img = whole_img(cv::Rect(begin.x, begin.y, whole_img.cols / 2, whole_img.rows / 2));

	// search maximum on image
	uchar max_intensity = 0;
	
	for (int i = 0; i < quarter_img.rows; ++i) {
		for (int j = 0; j < quarter_img.cols; ++j) {
			uchar next_intensity = quarter_img.at<uchar>(i, j);

			if ( next_intensity > max_intensity) {
				max_intensity = next_intensity;
			}
		}
	}


	// perform binarization of image with threshold as 0.8 from max intensity
	double threshold = std::max(static_cast<int>( max_intensity  * 0.8 + 0.5), 127);
	cv::Mat binImage(quarter_img.rows, quarter_img.cols, (uint8_t)0);
	cv::threshold(quarter_img, binImage, threshold, 255, CV_THRESH_BINARY );


	// find countors of image. Use 'CV_CHAIN_APPROX_NONE' to keep all points. Use CV_RETR_LIST since we don't care about hierarchy
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	findContours(binImage, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	// calculating and return properties for each countour 
	return getContoursProperties(contours, begin, imageCenterPos, quarter_idx);

}