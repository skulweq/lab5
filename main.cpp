#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

struct Color {
	int r, g, b;
	Color(int r = 0, int g = 0, int b = 0) : r(r), g(g), b(b) {}
};

Color averageColor(const std::vector<std::vector<Color>>& image, int x, int y) {
	int r = 0, g = 0, b = 0;
	int count = 0;

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			int newx = x + i, newy = y + j;
			if (newx >= 0 && newx < image.size() && newy >= 0 && newy < image[0].size()) {
				r += image[newx][newy].r;
				g += image[newx][newy].g;
				b += image[newx][newy].b;
				count++;
			}
		}
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
	std::vector<std::vector<Color>> image(1000, std::vector<Color>(1000, Color(100, 150, 200)));



	auto start1 = std::chrono::high_resolution_clock::now();
	auto blur1 = sequentialBlur(image);
	auto stop1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> blur1_time = stop1 - start1;
	std::cout << "sequentialBlur time: " << blur1_time.count() << std::endl;

	auto start2 = std::chrono::high_resolution_clock::now();
	auto blur2 = parallelBlurThreads(image, 8);
	auto stop2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> blur2_time = stop2 - start2;
	std::cout << "parallelBlurThreads time: " << blur2_time.count() << std::endl;

}