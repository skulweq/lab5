#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Color {
	int r, g, b;
	Color(int r = 0, int g = 0, int b = 0) : r(r), g(g), b(b) {}
};

std::vector<std::vector<Color>> loadImage(const std::string& filename, int& width, int& height) {
	int channels;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
	if (!data) {
		throw std::runtime_error("Failed to load image");
	}

	std::vector<std::vector<Color>> image(height, std::vector<Color>(width));
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = (i * width + j) * 3;
			image[i][j] = Color(data[index], data[index + 1], data[index + 2]);
		}
	}

	stbi_image_free(data);
	return image;
}

void saveImage(const std::string& filename, const std::vector<std::vector<Color>>& image) {
	int width = image[0].size();
	int height = image.size();
	std::vector<unsigned char> data(width * height * 3);

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = (i * width + j) * 3;
			data[index] = image[i][j].r;
			data[index + 1] = image[i][j].g;
			data[index + 2] = image[i][j].b;
		}
	}

	stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}

Color averageColor(const std::vector<std::vector<Color>>& image, int x, int y) {
	int r = 0, g = 0, b = 0;
	int count = 0;
	int kernel = 3;

	for (int i = -kernel; i <= kernel; i++) {
		for (int j = -kernel; j <= kernel; j++) {
			int newx = x + i, newy = y + j;
			if (newx >= 0 && newx < image.size() &&
				newy >= 0 && newy < image[0].size()) {
				r += image[newx][newy].r;
				g += image[newx][newy].g;
				b += image[newx][newy].b;
				count++;
			}
		}
	}

	if (count == 0) {
		return image[x][y];
	}
	return Color(r / count, g / count, b / count);
}

std::vector<std::vector<Color>> sequentialBlur(const std::vector<std::vector<Color>>& image) {
	std::vector<std::vector<Color>> result = image;

	for (int i = 0; i < image.size(); i++) {
		for (int j = 0; j < image[i].size(); j++) {
			result[i][j] = averageColor(image, i, j);
		}
	}
	return result;
}

std::vector<std::vector<Color>> parallelBlurThreads(std::vector<std::vector<Color>>& image, int threads_amount) {
	std::vector<std::vector<Color>> result = image;
	int rows_amount = image.size() / threads_amount;
	std::mutex mutex_;

	std::vector<std::thread> threads;

	auto blur = [&image, &mutex_, &result](auto start, auto end) {
		for (int i = start; i < end; i++) {
			for (int j = 0; j < image[i].size(); j++) {
				result[i][j] = averageColor(image, i, j);
			}
		}
	};

	for (int p = 0; p < threads_amount; p++) {
		int start = p * rows_amount;
		int end = (p == threads_amount - 1) ? image.size() : (p + 1) * rows_amount;
		threads.push_back(std::thread(blur, start, end));
	}

	for (auto& threads_ : threads) {
		threads_.join();
	}

	return result;
}



int main() {
	try {
		int width, height;
		std::string filename = "picture.jpg";
		auto image = loadImage(filename, width, height);


		//std::vector<std::vector<Color>> image(1000, std::vector<Color>(1000, Color(100, 150, 200)));



		auto start1 = std::chrono::high_resolution_clock::now();
		auto blur1 = sequentialBlur(image);
		auto stop1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> blur1_time = stop1 - start1;
		std::cout << "sequentialBlur time: " << blur1_time.count() << std::endl;
		saveImage("blur1.jpg", blur1);

		auto start2 = std::chrono::high_resolution_clock::now();
		auto blur2 = parallelBlurThreads(image, 8);
		auto stop2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> blur2_time = stop2 - start2;
		std::cout << "parallelBlurThreads time: " << blur2_time.count() << std::endl;
		saveImage("blur2.jpg", blur2);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}