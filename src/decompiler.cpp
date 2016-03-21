#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>
#include <json/json.h>
#include "psb.hpp"
#include "compress.h"

using namespace std;

#define BMP_BIT 32

#pragma pack(1)

struct BITMAP_FILE_HDR {
	unsigned short    bfType;
	unsigned int	  bfSize;
	unsigned short    bfReserved1;
	unsigned short    bfReserved2;
	unsigned int	  bfOffBits;
};

struct BITMAP_INFO_HDR {
	unsigned int      biSize;
	unsigned int      biWidth;
	unsigned int      biHeight;
	unsigned short    biPlanes;
	unsigned short    biBitCount;
	unsigned int      biCompression;
	unsigned int      biSizeImage;
	unsigned int      biXPelsPerMeter;
	unsigned int      biYPelsPerMeter;
	unsigned int      biClrUsed;
	unsigned int      biClrImportant;
};
#pragma pack()

bool save_bmp(string filename, int width, int height, void *data)
{
	int nAlignWidth = width * 4;
	BITMAP_FILE_HDR Header;
	BITMAP_INFO_HDR HeaderInfo;
	Header.bfType = 0x4D42;
	Header.bfReserved1 = 0;
	Header.bfReserved2 = 0;
	Header.bfOffBits = (sizeof(BITMAP_FILE_HDR) + sizeof(BITMAP_INFO_HDR));
	Header.bfSize = (sizeof(BITMAP_FILE_HDR) + sizeof(BITMAP_INFO_HDR) + nAlignWidth* height * 4);
	HeaderInfo.biSize = sizeof(BITMAP_INFO_HDR);
	HeaderInfo.biWidth = width;
	HeaderInfo.biHeight = height;
	HeaderInfo.biPlanes = 1;
	HeaderInfo.biBitCount = BMP_BIT;
	HeaderInfo.biCompression = 0;
	HeaderInfo.biSizeImage = nAlignWidth * height;
	HeaderInfo.biXPelsPerMeter = 0;
	HeaderInfo.biYPelsPerMeter = 0;
	HeaderInfo.biClrUsed = 0;
	HeaderInfo.biClrImportant = 0;
	fstream output(filename + ".bmp", ios::binary | ios::out);

	output.write((const char *)&Header, sizeof(Header));
	output.write((const char *)&HeaderInfo, sizeof(HeaderInfo));
	output.write((const char *)data, HeaderInfo.biSizeImage);
	output.flush();
	output.close();
	return true;
}

struct file_object
{
	string name;
	psb_resource_t *resource;
};

void convert_pixel(unsigned char *data, int width, int height, int align)
{
	int widthSize = width * align;

	unsigned char *temp = new unsigned char[widthSize * height];

	for (int i = 0; i < height; i++)
	{
		unsigned char *dst = &temp[((height - i) - 1) * widthSize];
		unsigned char *src = &data[i * widthSize];
		memcpy(dst, src, widthSize);
	}

	memcpy(data, temp, widthSize * height);

	delete[] temp;
}

void
traversal_object_tree(psb_t& psb,
	const psb_objects_t *objects, string object_name, Json::Value &root);

void
traversal_offsets_tree(psb_t& psb,
	const psb_collection_t *offsets, string entry_name, Json::Value &root) {
	psb_value_t *value = NULL;

	for (uint32_t i = 0; i < offsets->size(); i++) {
		unsigned char* entry_buff = offsets->get(i);
		psb.unpack(value, entry_buff);


		if (value != NULL) {
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_COLLECTION) {

				Json::Value node(Json::arrayValue);
				traversal_offsets_tree(psb, (const psb_collection_t *)value, entry_name, node);
				root.append(node);

			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_OBJECTS) {

				Json::Value node(Json::objectValue);
				traversal_object_tree(psb, (const psb_objects_t *)value, entry_name, node);
				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_TRUE || value->get_type() == psb_value_t::TYPE_FALSE) {

				Json::Value node(Json::booleanValue);
				psb_boolean_t *psb_boolean = (psb_boolean_t*)value;
				node = psb_boolean->get_boolean();
				root.append(node);
			}

			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_STRING_N1) {

				Json::Value node(Json::stringValue);
				psb_string_t *psb_string = (psb_string_t*)value;
				node = psb_string->get_string();
				root.append(node);
			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NUMBER_N0 || value->get_type() == psb_value_t::TYPE_NUMBER_N1 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N2 || value->get_type() == psb_value_t::TYPE_NUMBER_N3 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N4) {

				Json::Value node(Json::uintValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root.append(node);
			}


			if (value->get_type() == psb_value_t::TYPE_FLOAT0 || value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_DOUBLE) {

				Json::Value node(Json::realValue);

				psb_number_t *number = (psb_number_t*)value;
				if (value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_FLOAT0) {
					node = number->get_float();
				}
				if (value->get_type() == psb_value_t::TYPE_DOUBLE) {
					node = number->get_double();
				}

				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_NUMBER_N5 || value->get_type() == psb_value_t::TYPE_NUMBER_N6 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N7 || value->get_type() == psb_value_t::TYPE_NUMBER_N8) {

				Json::Value node(Json::uintValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer64();
				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_NULL) {
				Json::Value node(Json::nullValue);
				root.append(node);
			}
			if (value->get_type() == psb_value_t::TYPE_RESOURCE_N1 || value->get_type() == psb_value_t::TYPE_RESOURCE_N2 ||
				value->get_type() == psb_value_t::TYPE_RESOURCE_N3 || value->get_type() == psb_value_t::TYPE_RESOURCE_N4) {
				psb_resource_t *resource = (psb_resource_t *)value;
				Json::Value node(Json::stringValue);
				char temp[32];
				_itoa_s(resource->get_index(), temp, 10);
				node = "#resource#" + (string)temp;
				root.append(node);
			}
		}

		else {
			printf("invalid_type:%s,%02X\n", entry_name.c_str(), entry_buff[0]);
		}
	}
}
void
traversal_object_tree(psb_t& psb,
	const psb_objects_t *objects, string object_name, Json::Value &root) {
	psb_value_t *value = NULL;
	for (uint32_t i = 0; i < objects->size(); i++) {
		string entry_name = objects->get_name(i);
		unsigned char* entry_buff = objects->get_data(i);

		psb.unpack(value, entry_buff);

		if (value != NULL) {
			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_COLLECTION) {

				Json::Value node(Json::arrayValue);
				traversal_offsets_tree(psb, (const psb_collection_t *)value, entry_name, node);
				root[entry_name] = node;
			}
			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_OBJECTS) {

				Json::Value node(Json::objectValue);
				traversal_object_tree(psb, (const psb_objects_t *)value, entry_name, node);
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_TRUE || value->get_type() == psb_value_t::TYPE_FALSE) {

				Json::Value node(Json::booleanValue);
				psb_boolean_t *psb_boolean = (psb_boolean_t*)value;
				node = psb_boolean->get_boolean();
				root[entry_name] = node;
			}

			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_STRING_N1) {

				Json::Value node(Json::stringValue);
				psb_string_t *psb_string = (psb_string_t*)value;
				node = psb_string->get_string();
				root[entry_name] = node;
			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NUMBER_N0 || value->get_type() == psb_value_t::TYPE_NUMBER_N1 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N2 || value->get_type() == psb_value_t::TYPE_NUMBER_N3 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N4) {

				Json::Value node(Json::uintValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_FLOAT0 || value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_DOUBLE) {

				Json::Value node(Json::realValue);

				psb_number_t *number = (psb_number_t*)value;
				if (value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_FLOAT0) {
					node = number->get_float();
				}
				if (value->get_type() == psb_value_t::TYPE_DOUBLE) {
					node = number->get_double();
				}

				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_NUMBER_N5 || value->get_type() == psb_value_t::TYPE_NUMBER_N6 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N7 || value->get_type() == psb_value_t::TYPE_NUMBER_N8) {

				Json::Value node(Json::uintValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer64();
				root[entry_name] = node;
			}

			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NULL) {
				Json::Value node(Json::nullValue);
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_RESOURCE_N1 || value->get_type() == psb_value_t::TYPE_RESOURCE_N2 ||
				value->get_type() == psb_value_t::TYPE_RESOURCE_N3 || value->get_type() == psb_value_t::TYPE_RESOURCE_N4) {
				psb_resource_t *resource = (psb_resource_t *)value;

				Json::Value node(Json::stringValue);
				char temp[32];
				_itoa_s(resource->get_index(), temp, 10);
				node = "#resource#" + (string)temp;
				root[entry_name] = node;
			}
		}
		else {
			printf("invalid_type:%s:%02X\n", entry_name.c_str(), entry_buff[0]);
		}
	}
}


void export_res(psb_t &psb, string source_name)
{
	Json::Value res(Json::arrayValue);

	_mkdir("res");
	for (uint32_t i = 0; i < psb.chunk_offsets->size(); i++)
	{
		char filename[256];

		uint32_t offset = psb.chunk_offsets->get(i);
		uint32_t length = psb.chunk_lengths->get(i);

		sprintf_s(filename, "res/%d.bin", i);

		fstream output(filename, ios::trunc | ios::binary | ios::out);
		if (output.is_open())
		{
			output.write((const char*)psb.chunk_data + offset, length);
			output.flush();
			output.close();

			res[i] = filename;
		}
	}
	fstream output(source_name + ".res.json", ios::trunc | ios::out);
	output << res.toStyledString() << endl;
	output.flush(); output.close();
}
int main(int argc, char* argv[])
{
	Json::Value root;
	uint32_t pos;
	char* data;

	if(argc < 2)
	{
		cout << "usage: decompiler <psb_file>" << endl;
		return 0;
	}


	string filename = argv[1];
	fstream reader(filename, ios::binary | ios::in);
	fstream writer(filename + ".json", ios::out);

	if (!reader.is_open()) {
		cout << "open psb file failed" << endl;
		return 1;
	}
	if (!writer.is_open()) {
		cout << "open output file failed" << endl;
		return 1;
	}

	reader.seekg(0, ios::end);
	pos = (uint32_t)reader.tellg();
	reader.seekg(0, ios::beg);
	data = new char[pos];

	reader.read(data, pos);
	psb_t psb((unsigned char*)data);
	const psb_objects_t *objects = psb.get_objects();

	traversal_object_tree(psb, objects, "", root);
	writer << root.toStyledString() << endl;

	export_res(psb, filename);

	return 0;
}
