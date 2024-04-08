/**
* ��򵥵� FLV ��װ��ʽ��������
* Simplest FLV Parser
*
* ԭ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ����Ŀ��һ�� FLV ��װ��ʽ�������򣬿��Խ� FLV �е� MP3 ��Ƶ�������������
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

// �������fopen() ��������ȫ
#pragma warning(disable:4996)

#define PREVIOUS_TAG_SIZE_LENGTH 4

// Tag Type
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9
#define TAG_TYPE_SCRIPT 18

// 1 �ֽڣ�8 bit
typedef unsigned char UI8;
// 2 �ֽڣ�16 bit
typedef unsigned short UI16;
// 4 �ֽڣ�32 bit
typedef unsigned int UI32;

// FLV header��9 �ֽ�
typedef struct {
	UI8 Signature[3]; // 'F' (0x46)��'L' (0x4C)��'V' (0x56)
	UI8 Version; // FLV �汾��һ��Ϊ 0x01����ʾ FLV �汾 1
	UI8 TypeFlags; // b0 �Ƿ������Ƶ����b2 �Ƿ������Ƶ���������ֶα�����ֵΪ 0
	UI32 DataOffset; // FLV Header ���ȣ���λ���ֽڣ�һ��Ϊ 9
} FLVHEADER;

// FLV tag header��11 �ֽ�
typedef struct {
	UI8 TagType; // tag ���ͣ�8 = ��Ƶ��9 = ��Ƶ��18 = �ű�����
	UI8 DataSize[3]; // tag data ���ֵĴ�С�������� tag header
	UI8 Timestamp[3]; // ��ǰ tag �Ľ���ʱ��� (DTS)�����ֵ����λ��ms
	UI8 TimestampExtended; // �� Timestamp �ֶ�һ�𹹳�һ�� 32 λֵ, ���ֶ�Ϊ�� 8 λ����λ��ms
	UI8 StreamID[3]; // ���� 0
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
	// b2 �Ƿ������Ƶ����1 = ���ڣ�0 = ������
	fprintf(myout, "��Audio identification: %d��", (flv_header.TypeFlags & 0x04) >> 2);
	// b0 �Ƿ������Ƶ����1 = ���ڣ�0 = ������
	fprintf(myout, "Video identification: %d\n", flv_header.TypeFlags & 0x01);
	UI32 header_size = reverse_bytes((UI8 *)&flv_header.DataOffset, sizeof(flv_header.DataOffset));
	fprintf(myout, "HeaderSize: 0x%X\n", header_size);
	fprintf(myout, "================================================\n");

	// move the file pointer to the end of the header
	fseek(ifh, header_size, SEEK_SET); // SEEK_SET ��ʾ�ļ���ͷ

	// process each FLV tag
	do
	{
		/**
		* int _getw(FILE *stream)���Զ�������ʽ��ȡһ������
		* @param stream ָ�� FILE �ṹ��ָ��
		* @return ����������һ������ֵ
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
		// ��ǰ tag data ���ֵĴ�С�������� tag header
		int tagheader_datasize = tag_header.DataSize[0] * 65536 + tag_header.DataSize[1] * 256 + tag_header.DataSize[2];
		// ��ǰ tag �Ľ���ʱ��� (DTS)�����ֵ����λ��ms
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
			* int fgetc(FILE *stream)�����ļ�ָ�� stream ָ����ļ��ж�ȡһ���ַ�����ȡһ���ֽں󣬹��λ�ú���һ���ֽ�
			* @param stream ָ�� FILE �ṹ��ָ��
			* @return �������ж�ȡ��һ������ֵ
			*/
			UI8 audio_tag_header = fgetc(ifh);
			// audio tag header ռ 1 �ֽ�
			// ǰ 4 bit ��ʾ��Ƶ��ʽ
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
			// �� 5��6 bit ��ʾ������
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
			// �� 7 bit ��ʾ��������
			int SoundSize = (audio_tag_header & 0x02) >> 1;
			switch (SoundSize)
			{
			case 0: strcat(audio_tag_str, "8Bit"); break;
			case 1: strcat(audio_tag_str, "16Bit"); break;
			default: strcat(audio_tag_str, "unknown"); break;
			}
			strcat(audio_tag_str, "| ");
			// �� 8 bit ��ʾ��Ƶ����
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
				// �����Ƶ��ʽΪ 10������ AAC ��ʽ��
				// AudioTagHeader �л���һ���ֽ� AACPacketType������ֶ�����ʾ AACAUDIODATA �����ͣ�
				// 0 = AAC sequence header��1 = AAC raw��
				char aac_pakect_type[100] = { 0 };
				UI8 AACPacketType = fgetc(ifh);
				switch (AACPacketType)
				{
				case 0: strcat(aac_pakect_type, "AAC sequence header"); break;
				case 1: strcat(aac_pakect_type, "AAC raw"); break;
				default: strcat(aac_pakect_type, "unknown"); break;
				}
				fprintf(myout, "| %s", aac_pakect_type);
				// �ļ�ָ����� 1 �ֽ�
				fseek(ifh, -1, SEEK_CUR); // SEEK_CUR ��ʾ�ļ���ǰλ��
			}

			// if the output file hasn't been opened, open it.
			if (output_a != 0 && afh == NULL)
			{
				afh = fopen("output.mp3", "wb");
			}
			// audio tag data = tag_header.DataSize - 1���� tag body - audio tag header��1 �ֽڣ�
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
			// video tag header ռ 1 �ֽ�
			UI8 video_tag_header = fgetc(ifh);
			// ǰ 4 bit ��ʾ֡����
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
			// �� 4 bit ��ʾ��Ƶ��������
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
				// ��� CodecID = 7����Ƶ�ĸ�ʽ�� AVC��H.264���Ļ���
				// VideoTagHeader ���� 4 ���ֽڵ���Ϣ��AVCPacketType��1 �ֽڣ� �� CompositionTime��3 �ֽڣ�
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
				// �ļ�ָ����� 4 �ֽ�
				fseek(ifh, -4, SEEK_CUR);
			}

			// �ļ�ָ����� 1 �ֽ�
			fseek(ifh, -1, SEEK_CUR); // SEEK_CUR ��ʾ�ļ���ǰλ��
			// if the output file hasn't been opened, open it
			if (output_v != 0 && vfh == NULL)
			{
				vfh = fopen("output.flv", "wb");
				fwrite((char *)&flv_header, sizeof(flv_header), 1, vfh);
				// FLV header ��ĵ�һ�� PreviousTagSize������ 0
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
			fseek(ifh, -4, SEEK_CUR); // SEEK_CUR ��ʾ�ļ���ǰλ��

			break;
			}
		case TAG_TYPE_SCRIPT:
		{
			// ��һ�� AMF ��
			UI8 AMF1_Type = fgetc(ifh); // �� 1 ���ֽڱ�ʾ AMF �����ͣ�һ������ 0x02����ʾ�ַ���
			UI16 StringLength; // ��ʶ�ַ����ĳ��ȣ�һ������ 0x000A
			fread((char *)&StringLength, sizeof(UI16), 1, ifh);
			StringLength = reverse_bytes((UI8 *)&StringLength, sizeof(StringLength));
			char StringData[11]; // ������ַ�����һ����Ϊ "onMetaData"
			/**
			* char *fgets(char *str, int n, FILE *stream)
			* ��ָ������ stream ��ȡһ�У��������洢�� str ��ָ����ַ����ڡ�
			* ����ȡ (n-1) ���ַ�ʱ�����߶�ȡ�����з�ʱ�����ߵ����ļ�ĩβʱ������ֹͣ�����������������
			* @param str ָ��һ���ַ������ָ�룬������洢��Ҫ��ȡ���ַ�����
			* @param n Ҫ��ȡ������ַ������������Ŀ��ַ�����ͨ����ʹ���� str ���ݵ����鳤�ȡ�
			* @param stream ָ�� FILE �����ָ�룬�� FILE �����ʶ��Ҫ���ж�ȡ�ַ�������
			* @return ����ɹ����ú���������ͬ�� str ��������������ļ�ĩβ����û�ж�ȡ���κ��ַ���str ���䣬������һ����ָ�롣����������󣬷���һ����ָ�롣
			*/
			fgets(StringData, StringLength + 1, ifh);
			fprintf(myout, "\nAMF1 Type: 0x%X, StringLength: 0x%X, StringData: %s\n", AMF1_Type, StringLength, StringData);

			// �ڶ��� AMF ��
			UI8 AMF2_Type = fgetc(ifh); // �� 1 ���ֽڱ�ʾ AMF �����ͣ�һ������ 0x08����ʾ ECMA ����
			UI32 ECMAArrayLength; // ECMA ����Ԫ������
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