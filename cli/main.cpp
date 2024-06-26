#include "AtlasGenerator/Generator.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include "AtlasGenerator/PackagingException.h"
using namespace sc;

#define print(message) std::cout << message << std::endl

SC_CONSTRUCT_CHILD_EXCEPTION(GeneralRuntimeException, FolderAlreadyExistException, "Output folder already exists");

void print_help(char* executable)
{
	print("Sc Atlas Generator Command Line App: ");
	print("Usage: " << executable << " [Folder name with output] ...args");
	print("Arguments: Paths to images or to folder with image files");
	print("Flags: ");
	print("--force: rewrite output folder even if it already exists");
	print("--debug: draws and shows atlas of polygons and atlas itself");
	print("--item-debug: draws and shows polygon for each item");
}

class ProgramOptions
{
public:
	ProgramOptions(int argc, char* argv[])
	{
		output = argv[1];

		for (int i = last_argument_index(); argc > i; i++)
		{
			std::string argument = argv[i];

			// Flags
			if (argument == "--force")
			{
				force_output = true;
				continue;
			}

			if (argument == "--debug")
			{
				is_debug = true;
				continue;
			}

			if (argument == "--item-debug")
			{
				is_item_debug = true;
				continue;
			}

			// Paths
			if (!fs::exists(argument))
			{
				print("Unknown or wrong agument" << argument);
			}

			if (fs::is_directory(argument))
			{
				for (fs::path path : fs::directory_iterator(argument))
				{
					files.push_back(path);
				}
			}
			else
			{
				files.push_back(argument);
			}
		}
	}

	static int last_argument_index()
	{
		//	  0       1		2
		// File.exe output paths
		return 2;
	}

public:
	fs::path output;
	std::vector<fs::path> files;
	bool force_output = false;
	bool is_debug = false;
	bool is_item_debug = false;
};

#pragma region CV Debug Functions

void ShowImage(std::string name, cv::Mat& image) {
	cv::namedWindow(name, cv::WINDOW_NORMAL);

	cv::imshow(name, image);
	cv::waitKey(0);
}

void ShowContour(cv::Mat& src, std::vector<cv::Point>& points) {
	cv::Mat drawing = src.clone();
	drawContours(
		drawing,
		std::vector<std::vector<cv::Point>>(1, points),
		0,
		cv::Scalar(255, 255, 0),
		5,
		cv::LINE_AA
	);

	for (cv::Point& point : points) {
		circle(
			drawing,
			point,
			2,
			{ 0, 0, 255 },
			2,
			cv::LINE_AA
		);
	}
	ShowImage("Image polygon", drawing);
	cv::destroyAllWindows();
}

void ShowContour(cv::Mat& src, std::vector<AtlasGenerator::Vertex>& points) {
	std::vector<cv::Point> cvPoints;
	for (auto& point : points) {
		cvPoints.push_back({ point.uv.x, point.uv.y });
	}

	ShowContour(src, cvPoints);
}
#pragma endregion

void process(ProgramOptions& options)
{
	if (fs::exists(options.output) && fs::is_directory(options.output))
	{
		if (options.force_output)
		{
			fs::remove_all(options.output);
		}
		else
		{
			throw FolderAlreadyExistException();
		}
	}

	fs::create_directory(options.output);

	fs::path atlas_data_output = options.output / "atlas.txt";
	std::ofstream atlas_data(atlas_data_output);

	std::vector<AtlasGenerator::Item> items;

	std::map<size_t, Rect<int32_t>> guides;
	std::map<size_t, AtlasGenerator::Item::Transformation> guide_transform;

	for (fs::path& path : options.files)
	{
		if (path.extension() != ".png") continue;

		fs::path guide_path = fs::path(path).replace_extension().concat("_guide.txt");

		if (!fs::exists(guide_path))
		{
			items.emplace_back(path);
		}
		else
		{
			std::vector<float> guide;
			std::ifstream guide_file(guide_path);

			std::string line;
			while (std::getline(guide_file, line, '\n'))
			{
				guide.push_back(
					std::stof(line)
				);
			}

			if (guide.size() != 4) continue;

			guides[items.size()] = Rect<int32_t>(
				(int32_t)ceil(guide[0]), (int32_t)ceil(guide[3]),
				(int32_t)ceil(guide[1]), (int32_t)ceil(guide[2])
				);

			AtlasGenerator::Item item(path, AtlasGenerator::Item::Type::Sliced);

			AtlasGenerator::Item::Transformation transform(
				0.0,
				Point<int32_t>(-(item.width() / 2), -(item.height() / 2))
			);

			guide_transform[items.size()] = transform;

			items.push_back(item);
		}
	}
	uint8_t scale_factor = 2;
	AtlasGenerator::Config config(
		AtlasGenerator::Config::TextureType::RGBA,
		4096, 4096,
		scale_factor, 2
	);

	config.progress = [&items](unsigned count)
	{
		print(count << " \\ " << items.size());
	};

	AtlasGenerator::Generator generator(config);
	uint8_t bin_count = 0;
	try
	{
		bin_count = generator.generate(items);
	}
	catch (const AtlasGenerator::PackagingException& exception)
	{
		size_t item_index = exception.index();
		if (item_index == SIZE_MAX)
		{
			std::cout << "Unknown package exception" << std::endl;
		}
		else
		{
			std::cout << "Failed to package item \"" << options.files[item_index] << "\"" << std::endl;
		}
		std::cout << exception.message() << std::endl;
	}
	catch (const sc::GeneralRuntimeException& exception)
	{
		std::cout << exception.message() << std::endl;
	}

	for (uint8_t i = 0; bin_count > i; i++)
	{
		cv::Mat& image = generator.get_atlas(i);

		cv::imwrite(
			fs::path(options.output / fs::path("atlas_").concat(std::to_string(i)).concat(".png")).string(),
			image
		);
	}

	for (size_t i = 0; items.size() > i; i++)
	{
		AtlasGenerator::Item& item = items[i];
		fs::path& path = options.files[i];

		atlas_data << "path=" << path << std::endl;
		atlas_data << "textureIndex=" << std::to_string(item.texture_index) << std::endl;

		atlas_data << "uv=";
		for (AtlasGenerator::Vertex vertex : item.vertices)
		{
			item.transform.transform_point(vertex.uv);
			atlas_data << "[";
			atlas_data << std::to_string(vertex.uv.x / scale_factor);
			atlas_data << ",";
			atlas_data << std::to_string(vertex.uv.y / scale_factor);
			atlas_data << "]";
		}

		atlas_data << std::endl;

		atlas_data << "xy=";
		for (AtlasGenerator::Vertex& vertex : item.vertices)
		{
			atlas_data << "[";
			atlas_data << std::to_string(vertex.xy.x);
			atlas_data << ",";
			atlas_data << std::to_string(vertex.xy.y);
			atlas_data << "]";
		}

		atlas_data << std::endl << std::endl;
	}

	if (options.is_debug)
	{
		std::vector<cv::Mat> sheets;
		cv::RNG rng = cv::RNG(time(NULL));

		for (uint8_t i = 0; bin_count > i; i++) {
			cv::Mat& atlas = generator.get_atlas(i);
			sheets.emplace_back(
				atlas.size(),
				CV_8UC4,
				cv::Scalar(0)
			);
		}

		for (size_t i = 0; items.size() > i; i++) {
			AtlasGenerator::Item& item = items[i];
			std::vector<cv::Point> atlas_contour;
			std::vector<cv::Point> item_contour;

			for (AtlasGenerator::Vertex vertex : item.vertices) {
				item.transform.transform_point(vertex.uv);

				atlas_contour.push_back(cv::Point(vertex.uv.x, vertex.uv.y));
				item_contour.push_back(cv::Point(vertex.xy.x, vertex.xy.y));
			}

			{
				cv::Scalar color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
				fillPoly(sheets[item.texture_index], atlas_contour, color);
			}

			if (options.is_item_debug)
			{
				cv::Mat image_contour(item.image().size(), CV_8UC4, cv::Scalar(0));
				{
					cv::Scalar color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
					fillPoly(image_contour, item_contour, color);
				}

				std::vector<cv::Mat> matrices = {
					image_contour, item.image()
				};

				if (item.is_sliced())
				{
					cv::Mat sliced_image(item.image().size(), CV_8UC4, cv::Scalar(0));
					for (uint8_t area_index = (uint8_t)AtlasGenerator::Item::SlicedArea::BottomLeft; (uint8_t)AtlasGenerator::Item::SlicedArea::TopRight >= area_index; area_index++)
					{
						Rect<int32_t> xy;
						Rect<uint16_t> uv;

						item.get_sliced_area(
							(AtlasGenerator::Item::SlicedArea)area_index,
							guides[i],
							xy, uv,
							guide_transform[i]
						);

						if (xy.width > 0 && xy.height > 0)
						{
							{
								cv::Scalar color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));

								auto points = std::vector<cv::Point>{
										cv::Point(xy.x - guide_transform[i].translation.x, xy.y - guide_transform[i].translation.y),
										cv::Point((xy.x - guide_transform[i].translation.x) + xy.width, xy.y - guide_transform[i].translation.y),
										cv::Point((xy.x - guide_transform[i].translation.x) + xy.width, (xy.y - guide_transform[i].translation.y) + xy.height),
										cv::Point(xy.x - guide_transform[i].translation.x, (xy.y - guide_transform[i].translation.y) + xy.height),
								};
								fillPoly(
									sliced_image,
									points,
									color
								);
							}

							{
								auto points = std::vector<cv::Point>{
										cv::Point(uv.x, uv.y),
										cv::Point(uv.x + uv.width, uv.y),
										cv::Point(uv.x + uv.width, uv.y + uv.height),
										cv::Point(uv.x, uv.y + uv.height)
								};

								cv::Scalar color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
								fillPoly(sheets[item.texture_index], points, color);
							}
						}
					}
					matrices.push_back(sliced_image);
				}

				{
					cv::Mat canvas;
					cv::hconcat(matrices, canvas);
					ShowImage("Item", canvas);
				}
			}
		}

		for (cv::Mat& sheet : sheets) {
			ShowImage("Sheet", sheet);
		}

		for (uint8_t i = 0; bin_count > i; i++) {
			ShowImage("Atlas", generator.get_atlas(i));
		}

		if (options.is_item_debug)
		{
		}

		cv::destroyAllWindows();
	}
}

int main(int argc, char* argv[])
{
	if (argc <= ProgramOptions::last_argument_index())
	{
		print_help(argv[0]);
		return 1;
	}

	ProgramOptions options(argc, argv);

	try
	{
		process(options);
	}
	catch (const GeneralRuntimeException& exception)
	{
		print(exception.message());
		return 1;
	}

	return 0;
}