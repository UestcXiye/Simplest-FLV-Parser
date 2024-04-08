/**
* 最简单的 FLV 封装格式解析程序
* Simplest FLV Parser
*
* 原程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本项目是一个 FLV 封装格式解析程序，可以将 FLV 中的 MP3 音频码流分离出来。
*
* This project is a FLV format analysis program.
* It can analysis FLV file and extract MP3 audio stream.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Important
#pragma pack(1)

// 解决报错：fopen() 函数不安全
#pragma warning(disable:4996)

#define PREVIOUS_TAG_SIZE_LENGTH 4

// Tag Type
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9
#define TAG_TYPE_SCRIPT 18

// 1 字节，8 bit
typedef unsigned char UI8;
// 2 字节，16 bit
typedef unsigned short UI16;
// 4 字节，32 bit
typedef unsigned int UI32;

// FLV header，9 字节
typedef struct {
	UI8 Signature[3]; // 'F' (0x46)，'L' (0x4C)，'V' (0x56)
	UI8 Version; // FLV 版本，一般为 0x01，表示 FLV 版本 1
	UI8 TypeFlags; // b0 是否存在视频流，b2 是否存在音频流，其他字段保留，值为 0
	UI32 DataOffset; // FLV Header 长度，单位：字节，一般为 9
} FLVHEADER;

// FLV tag header，11 字节
typedef struct {
	UI8 TagType; // tag 类型，8 = 音频，9 = 视频，18 = 脚本数据
	UI8 DataSize[3]; // tag data 部分的大小，不包含 tag header
	UI8 Timestamp[3]; // 当前 tag 的解码时间戳 (DTS)，相对值，单位：ms
	UI8 TimestampExtended; // 和 Timestamp 字段一起构成一个 32 位值, 此字段为高 8 位，单位：ms
	UI8 StreamID[3]; // 总是 0
} TAGHEADER;

// turn a BigEndian byte array into a LittleEndian integer
unsigned int reverse_bytes(UI8 *p, char c)
{
	int r = 0;
	int i;
	for (i = 0; i < c; i++)
		r |= (*(p + i) << (((c - 1) * 8) - 8 * i));
	return r;
}

/**
* Analysis FLV file
* @param url Location of input FLV file
*/
int simplest_flv_parser(char *url)
{
	// whether output audio / video stream
	int output_a = 1;
	int output_v = 1;
	FILE *ifh = NULL, *vfh = NULL, *afh = NULL;

	// FILE *myout = fopen("output_log.txt", "wb+");
	FILE *myout = stdout;

	FLVHEADER flv_header;
	TAGHEADER tag_header;

	UI32 PreviousTagSize, PreviousTagSize_z = 0;
	UI32 ts = 0, ts_new = 0;

	ifh = fopen(url, "rb+");
	if (ifh == NULL)
	{
		printf("Failed to open files.\n");
		return -1;
	}

	// read FLV header
	fread((char *)&flv_header, sizeof(FLVHEADER), 1, ifh);
	// print FLV header information
	fprintf(myout, "================== FLV Header ==================\n");
	fprintf(myout, "Signature: %c %c %c\n", flv_header.Signature[0], flv_header.Signature[1], flv_header.Signature[2]);
	fprintf(myout, "Version: 0x%X\n", flv_header.Version);
	fprintf(myout, "TypeFlags: 0x%X", flv_header.TypeFlags);
	// b2 是否存在音频流，1 = 存在，0 = 不存在
	fprintf(myout, "，Audio identification: %d，", (flv_header.TypeFlags & 0x04) >> 2);
	// b0 是否存在视频流，1 = 存在，0 = 不存在
	fprintf(myout, "Video identification: %d\n", flv_header.TypeFlags & 0x01);
	UI32 header_size = reverse_bytes((UI8 *)&flv_header.DataOffset, sizeof(flv_header.DataOffset));
	fprintf(myout, "HeaderSize: 0x%X\n", header_size);
	fprintf(myout, "================================================\n");

	// move the file pointer to the end of the header
	fseek(ifh, header_size, SEEK_SET); // SEEK_SET 表示文件开头

	// process each FLV tag
	do
	{
		/**
		* int _getw(FILE *stream)：以二进制形式读取一个整数
		* @param stream 指向 FILE 结构的指针
		* @return 返回流中下一个整数值
		*/
		PreviousTagSize = _getw(ifh);
		// read FLV tag header
		fread((char *)&tag_header, sizeof(TAGHEADER), 1, ifh);

		char tagtype_str[10];
		switch (tag_header.TagType)
		{
		case TAG_TYPE_AUDIO:
			sprintf(tagtype_str, "Audio");
			break;
		case TAG_TYPE_VIDEO:
			sprintf(tagtype_str, "Video");
			break;
		case TAG_TYPE_SCRIPT:
			sprintf(tagtype_str, "Script");
			break;
		default:
			sprintf(tagtype_str, "unknown");
			break;
		}
		// 当前 tag data 部分的大小，不包含 tag header
		int tagheader_datasize = tag_header.DataSize[0] * 65536 + tag_header.DataSize[1] * 256 + tag_header.DataSize[2];
		// 当前 tag 的解码时间戳 (DTS)，相对值，单位：ms
		int tagheader_timestamp = tag_header.Timestamp[0] * 65536 + tag_header.Timestamp[1] * 256 + tag_header.Timestamp[2];
		fprintf(myout, "[%6s] %6d %6d |", tagtype_str, tagheader_datasize, tagheader_timestamp);

		// if we are not past the end of file, process the tag
		if (feof(ifh))
			break;

		// process tag by type
		switch (tag_header.TagType)
		{
		case TAG_TYPE_AUDIO:
		{
			char audio_tag_str[100] = { 0 };
			strcat(audio_tag_str, "| ");
			/**
			* int fgetc(FILE *stream)：从文件指针 stream 指向的文件中读取一个字符，读取一个字节后，光标位置后移一个字节
			* @param stream 指向 FILE 结构的指针
			* @return 返回流中读取的一个整数值
			*/
			UI8 audio_tag_header = fgetc(ifh);
			// audio tag header 占 1 字节
			// 前 4 bit 表示音频格式
			int SoundFormat = (audio_tag_header & 0xF0) >> 4;
			switch (SoundFormat)
			{
			case 0: strcat(audio_tag_str, "Linear PCM, platform endian"); break;
			case 1: strcat(audio_tag_str, "ADPCM"); break;
			case 2: strcat(audio_tag_str, "MP3"); break;
			case 3: strcat(audio_tag_str, "Linear PCM, little endian"); break;
			case 4: strcat(audio_tag_str, "Nellymoser 16-kHz mono"); break;
			case 5: strcat(audio_tag_str, "Nellymoser 8-kHz mono"); break;
			case 6: strcat(audio_tag_str, "Nellymoser"); break;
			case 7: strcat(audio_tag_str, "G.711 A-law logarithmic PCM"); break;
			case 8: strcat(audio_tag_str, "G.711 mu-law logarithmic PCM"); break;
			case 9: strcat(audio_tag_str, "reserved"); break;
			case 10: strcat(audio_tag_str, "AAC"); break;
			case 11: strcat(audio_tag_str, "Speex"); break;
			case 14: strcat(audio_tag_str, "MP3 8-Khz"); break;
			case 15: strcat(audio_tag_str, "Device-specific sound"); break;
			default: strcat(audio_tag_str, "unknown"); break;
			}
			strcat(audio_tag_str, "| ");
			// 第 5、6 bit 表示采样率
			int SoundRate = (audio_tag_header & 0x0C) >> 2;
			switch (SoundRate)
			{
			case 0: strcat(audio_tag_str, "5.5-kHz"); break;
			case 1: strcat(audio_tag_str, "11-kHz"); break;
			case 2: strcat(audio_tag_str, "22-kHz"); break;
			case 3: strcat(audio_tag_str, "44-kHz"); break;
			default: strcat(audio_tag_str, "unknown"); break;
			}
			strcat(audio_tag_str, "| ");
			// 第 7 bit 表示采样精度
			int SoundSize = (audio_tag_header & 0x02) >> 1;
			switch (SoundSize)
			{
			case 0: strcat(audio_tag_str, "8Bit"); break;
			case 1: strcat(audio_tag_str, "16Bit"); break;
			default: strcat(audio_tag_str, "unknown"); break;
			}
			strcat(audio_tag_str, "| ");
			// 第 8 bit 表示音频声道
			int SoundType = audio_tag_header & 0x01;
			switch (SoundType)
			{
			case 0: strcat(audio_tag_str, "Mono"); break;
			case 1: strcat(audio_tag_str, "Stereo"); break;
			default: strcat(audio_tag_str, "unknown"); break;
			}
			fprintf(myout, "%s", audio_tag_str);

			if (SoundFormat == 10)
			{
				// 如果音频格式为 10，即是 AAC 格式。
				// AudioTagHeader 中会多出一个字节 AACPacketType，这个字段来表示 AACAUDIODATA 的类型，
				// 0 = AAC sequence header，1 = AAC raw。
				char aac_pakect_type[100] = { 0 };
				UI8 AACPacketType = fgetc(ifh);
				switch (AACPacketType)
				{
				case 0: strcat(aac_pakect_type, "AAC sequence header"); break;
				case 1: strcat(aac_pakect_type, "AAC raw"); break;
				default: strcat(aac_pakect_type, "unknown"); break;
				}
				fprintf(myout, "| %s", aac_pakect_type);
				// 文件指针回退 1 字节
				fseek(ifh, -1, SEEK_CUR); // SEEK_CUR 表示文件当前位置
			}

			// if the output file hasn't been opened, open it.
			if (output_a != 0 && afh == NULL)
			{
				afh = fopen("output.mp3", "wb");
			}
			// audio tag data = tag_header.DataSize - 1，即 tag body - audio tag header（1 字节）
			int audio_tag_datasize = reverse_bytes((UI8 *)&tag_header.DataSize, sizeof(tag_header.DataSize)) - 1;
			if (output_a != 0)
			{
				for (int i = 0; i < audio_tag_datasize; i++)
					fputc(fgetc(ifh), afh);
			}
			else
			{
				for (int i = 0; i < audio_tag_datasize; i++)
					fgetc(ifh);
			}

			break;
		}
		case TAG_TYPE_VIDEO:
		{
			char video_tag_str[100] = { 0 };
			strcat(video_tag_str, "| ");
			// video tag header 占 1 字节
			UI8 video_tag_header = fgetc(ifh);
			// 前 4 bit 表示帧类型
			int FrameType = (video_tag_header & 0xF0) >> 4;
			switch (FrameType)
			{
			case 1: strcat(video_tag_str, "keyframe (for AVC, a seekable frame)"); break;
			case 2: strcat(video_tag_str, "inter frame (for AVC, a non-seekable frame)"); break;
			case 3: strcat(video_tag_str, "disposable inter frame (H.263 only)"); break;
			case 4: strcat(video_tag_str, "generated keyframe (reserved for server use only)"); break;
			case 5: strcat(video_tag_str, "video info/command frame"); break;
			default: strcat(video_tag_str, "unknown"); break;
			}
			strcat(video_tag_str, "| ");
			// 后 4 bit 表示视频编码类型
			int CodecId = video_tag_header & 0x0F;
			switch (CodecId)
			{
			case 1: strcat(video_tag_str, "JPEG (currently unused)"); break;
			case 2: strcat(video_tag_str, "Sorenson H.263"); break;
			case 3: strcat(video_tag_str, "Screen video"); break;
			case 4: strcat(video_tag_str, "On2 VP6"); break;
			case 5: strcat(video_tag_str, "On2 VP6 with alpha channel"); break;
			case 6: strcat(video_tag_str, "Screen video version 2"); break;
			case 7: strcat(video_tag_str, "AVC"); break;
			default: strcat(video_tag_str, "unknown"); break;
			}
			fprintf(myout, "%s", video_tag_str);

			if (CodecId == 7)
			{
				// 如果 CodecID = 7，视频的格式是 AVC（H.264）的话，
				// VideoTagHeader 会多出 4 个字节的信息，AVCPacketType（1 字节） 和 CompositionTime（3 字节）
				char avc_pakect_type[100] = { 0 };
				UI8 AVCPacketType = fgetc(ifh);
				switch (AVCPacketType)
				{
				case 0: strcat(avc_pakect_type, "AVCDecoderConfigurationRecord (AVC sequence header)"); break;
				case 1: strcat(avc_pakect_type, "AVC NALU"); break;
				case 2: strcat(avc_pakect_type, "AVC end of sequence (lower level NALU sequence ender is not required or supported)"); break;
				default: strcat(avc_pakect_type, "unknown"); break;
				}
				UI8 CTS[3];
				fread((char *)&CTS, sizeof(UI8), 3, ifh);
				int cts = reverse_bytes((UI8 *)&CTS, sizeof(CTS));
				fprintf(myout, "| %s| %d", avc_pakect_type, cts);
				// 文件指针回退 4 字节
				fseek(ifh, -4, SEEK_CUR);
			}

			// 文件指针回退 1 字节
			fseek(ifh, -1, SEEK_CUR); // SEEK_CUR 表示文件当前位置
			// if the output file hasn't been opened, open it
			if (output_v != 0 && vfh == NULL)
			{
				vfh = fopen("output.flv", "wb");
				fwrite((char *)&flv_header, sizeof(flv_header), 1, vfh);
				// FLV header 后的第一个 PreviousTagSize，总是 0
				fwrite((char *)&PreviousTagSize_z, 1, sizeof(PreviousTagSize_z), vfh);
			}
#if 0
			// Change Timestamp
			ts = reverse_bytes((UI8 *)&tag_header.Timestamp, sizeof(tag_header.Timestamp));
			ts *= 2;
			// Writeback Timestamp
			ts_new = reverse_bytes((UI8 *)&ts, sizeof(ts));
			memcpy(&tag_header.Timestamp, ((char *)&ts_new) + 1, sizeof(tag_header.Timestamp));
#endif
			int video_tag_datasize = reverse_bytes((UI8 *)&tag_header.DataSize, sizeof(tag_header.DataSize));
			// tag data + Previous Tag Size
			int data_size = video_tag_datasize + PREVIOUS_TAG_SIZE_LENGTH;
			if (output_v != 0)
			{
				// tag header
				fwrite((char *)&tag_header, 1, sizeof(tag_header), vfh);
				// tag data
				for (int i = 0; i < data_size; i++)
					fputc(fgetc(ifh), vfh);
			}
			else
			{
				for (int i = 0; i < data_size; i++)
					fgetc(ifh);
			}
			// rewind 4 bytes, because we need to read the previoustagsize again for the loop's sake
			fseek(ifh, -4, SEEK_CUR); // SEEK_CUR 表示文件当前位置

			break;
			}
		case TAG_TYPE_SCRIPT:
		{
			// 第一个 AMF 包
			UI8 AMF1_Type = fgetc(ifh); // 第 1 个字节表示 AMF 包类型，一般总是 0x02，表示字符串
			UI16 StringLength; // 标识字符串的长度，一般总是 0x000A
			fread((char *)&StringLength, sizeof(UI16), 1, ifh);
			StringLength = reverse_bytes((UI8 *)&StringLength, sizeof(StringLength));
			char StringData[11]; // 具体的字符串，一般总为 "onMetaData"
			/**
			* char *fgets(char *str, int n, FILE *stream)
			* 从指定的流 stream 读取一行，并把它存储在 str 所指向的字符串内。
			* 当读取 (n-1) 个字符时，或者读取到换行符时，或者到达文件末尾时，它会停止，具体视情况而定。
			* @param str 指向一个字符数组的指针，该数组存储了要读取的字符串。
			* @param n 要读取的最大字符数（包括最后的空字符）。通常是使用以 str 传递的数组长度。
			* @param stream 指向 FILE 对象的指针，该 FILE 对象标识了要从中读取字符的流。
			* @return 如果成功，该函数返回相同的 str 参数。如果到达文件末尾或者没有读取到任何字符，str 不变，并返回一个空指针。如果发生错误，返回一个空指针。
			*/
			fgets(StringData, StringLength + 1, ifh);
			fprintf(myout, "\nAMF1 Type: 0x%X, StringLength: 0x%X, StringData: %s\n", AMF1_Type, StringLength, StringData);

			// 第二个 AMF 包
			UI8 AMF2_Type = fgetc(ifh); // 第 1 个字节表示 AMF 包类型，一般总是 0x08，表示 ECMA 数组
			UI32 ECMAArrayLength; // ECMA 数组元素数量
			fread((char *)&ECMAArrayLength, sizeof(UI32), 1, ifh);
			ECMAArrayLength = reverse_bytes((UI8 *)&ECMAArrayLength, sizeof(ECMAArrayLength));
			fprintf(myout, "AMF2 Type: 0x%X, ECMAArrayLength: %d", AMF2_Type, ECMAArrayLength);
			fseek(ifh, -(3 + StringLength + 1 + 4), SEEK_CUR);
			fseek(ifh, reverse_bytes((UI8 *)&tag_header.DataSize, sizeof(tag_header.DataSize)), SEEK_CUR);
			break;
		}
		default:
			// skip this tag
			fseek(ifh, reverse_bytes((UI8 *)&tag_header.DataSize, sizeof(tag_header.DataSize)), SEEK_CUR);
			break;
		}
		fprintf(myout, "\n");
		} while (!feof(ifh));

		// _fcloseall();
		fclose(ifh);
		if (output_a)
			fclose(afh);
		if (output_v)
			fclose(vfh);

		return 0;
	}

int main()
{
	simplest_flv_parser("cuc_ieschool.flv");

	system("pause");
	return 0;
}