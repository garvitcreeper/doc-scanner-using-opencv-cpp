#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem> // For creating directories

using namespace cv;
using namespace std;

///////////////  Project 2 - Document Scanner  //////////////////////

Mat imgOriginal, imgGray, imgBlur, imgCanny, imgThre, imgDil, imgErode, imgWarp, imgCrop;
vector<Point> initialPoints, docPoints;
float w = 420, h = 596;

Mat preProcessing(Mat img)
{
    // Convert the image to grayscale
    cvtColor(img, imgGray, COLOR_BGR2GRAY);

    // Apply Gaussian blur to the grayscale image
    GaussianBlur(imgGray, imgBlur, Size(3, 3), 3, 0);

    // Detect edges using the Canny edge detector
    Canny(imgBlur, imgCanny, 25, 75);

    // Create a structuring element for dilation
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

    // Dilate the edges to enhance them
    dilate(imgCanny, imgDil, kernel);

    return imgDil;
}

vector<Point> getContours(Mat image) {

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // Find external contours in the image
    findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<vector<Point>> conPoly(contours.size());

    vector<Point> biggest;
    int maxArea = 0;

    for (int i = 0; i < contours.size(); i++)
    {
        int area = contourArea(contours[i]);

        // Only consider contours with area greater than 1000
        if (area > 1000)
        {
            float peri = arcLength(contours[i], true);
            approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);

            // Check if the contour has 4 points and is the largest one found so far
            if (area > maxArea && conPoly[i].size() == 4) {
                biggest = { conPoly[i][0], conPoly[i][1], conPoly[i][2], conPoly[i][3] };
                maxArea = area;
            }
        }
    }
    return biggest;
}

void drawPoints(vector<Point> points, Scalar color)
{
    // Draw circles and index numbers on the detected points
    for (int i = 0; i < points.size(); i++)
    {
        circle(imgOriginal, points[i], 10, color, FILLED);
        putText(imgOriginal, to_string(i), points[i], FONT_HERSHEY_PLAIN, 4, color, 4);
    }
}

vector<Point> reorder(vector<Point> points)
{
    vector<Point> newPoints;
    vector<int> sumPoints, subPoints;

    for (int i = 0; i < 4; i++)
    {
        sumPoints.push_back(points[i].x + points[i].y);
        subPoints.push_back(points[i].x - points[i].y);
    }

    newPoints.push_back(points[min_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); // 0
    newPoints.push_back(points[max_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]); // 1
    newPoints.push_back(points[min_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]); // 2
    newPoints.push_back(points[max_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); // 3

    return newPoints;
}

Mat getWarp(Mat img, vector<Point> points, float w, float h)
{
    Point2f src[4] = { points[0], points[1], points[2], points[3] };
    Point2f dst[4] = { {0.0f, 0.0f}, {w, 0.0f}, {0.0f, h}, {w, h} };

    // Get the perspective transform matrix and warp the image
    Mat matrix = getPerspectiveTransform(src, dst);
    warpPerspective(img, imgWarp, matrix, Point(w, h));

    return imgWarp;
}

int main() {

    string path = "resources/sample.jpg";
    imgOriginal = imread(path);

    // Check if the image is loaded successfully
    if (imgOriginal.empty()) {
        cout << "Could not read the image: " << path << endl;
        return 1;
    }

    // Preprocess the image
    imgThre = preProcessing(imgOriginal);

    // Get the contours of the document
    initialPoints = getContours(imgThre);
    if (initialPoints.size() == 0) {
        cout << "No document detected!" << endl;
        return 1;
    }

    // Reorder the points for the perspective transform
    docPoints = reorder(initialPoints);

    // Warp the image to get a top-down view of the document
    imgWarp = getWarp(imgOriginal, docPoints, w, h);

    // Crop the warped image slightly to remove potential black edges
    int cropVal = 5;
    Rect roi(cropVal, cropVal, w - (2 * cropVal), h - (2 * cropVal));
    imgCrop = imgWarp(roi);

    // Display the images
    imshow("Original Image", imgOriginal);
    imshow("Threshold Image", imgThre);
    imshow("Warped Image", imgWarp);
    imshow("Cropped Image", imgCrop);

    // Save the output images in the same folder
    string outputFolder = "output/";
    filesystem::create_directory(outputFolder);
    imwrite(outputFolder + "Original_Image.jpg", imgOriginal);
    imwrite(outputFolder + "Threshold_Image.jpg", imgThre);
    imwrite(outputFolder + "Warped_Image.jpg", imgWarp);
    imwrite(outputFolder + "Cropped_Image.jpg", imgCrop);

    waitKey(0);

    return 0;
}
