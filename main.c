#include <stdio.h>
#include <stdlib.h>
#include "LzmaLib.h"
#include <Windows.h>
#include <string.h>

int main()
{
    printf("LZMA Compression ------------\n");

    while (1)
    {
        FILE *f;
        unsigned char *buf;
        size_t bufSize, processedSize, uncompressedSize;
        int res;
        char filename[256];
        char outputFilename[256];

        printf("Enter 'c' to compress or 'd' to decompress: ");
        int operation = getchar();
        while (getchar() != '\n')
            ; // Clear the input buffer

        printf("Enter the file name: ");
        fgets(filename, sizeof(filename), stdin);
        size_t len = strlen(filename);
        if (len > 0 && filename[len - 1] == '\n')
        {
            filename[len - 1] = '\0';
        }

        printf("Enter the output file name: ");
        fgets(outputFilename, sizeof(outputFilename), stdin);
        len = strlen(outputFilename);
        if (len > 0 && outputFilename[len - 1] == '\n')
        {
            outputFilename[len - 1] = '\0';
        }

        f = fopen(filename, "rb");
        if (f == NULL)
        {
            printf("Cannot open file '%s'.\n", filename);
            continue;
        }

        // Read the file into memory
        fseek(f, 0, SEEK_END);
        bufSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        buf = (unsigned char *)malloc(bufSize);
        if (buf == NULL)
        {
            printf("Cannot allocate memory.\n");
            fclose(f);
            continue;
        }

        processedSize = fread(buf, 1, bufSize, f);
        fclose(f);
        if (processedSize != bufSize)
        {
            printf("Cannot read file.\n");
            free(buf);
            continue;
        }

        if (operation == 'c')
        {
            size_t outPropsSize = LZMA_PROPS_SIZE;
            size_t destLen = bufSize + outPropsSize + (bufSize / 3) + 128;
            unsigned char outProps[LZMA_PROPS_SIZE];
            unsigned char *outBuf = (unsigned char *)malloc(destLen);

            if (outBuf == NULL)
            {
                printf("Cannot allocate memory for the output buffer.\n");
                free(buf);
                continue;
            }

            int fb;
            printf("Enter fb (5 to 273): ");
            scanf("%d", &fb);
            while (getchar() != '\n')
                ; // Clear the input buffer

            if (!(fb >= 5 && fb <= 273))
            {
                printf("Enter valid number in range 5 to 273.\n");
                free(buf);
                free(outBuf);
                continue;
            }
            res = LzmaCompress(
                outBuf + outPropsSize + sizeof(size_t), &destLen, // Allocate the first 5 + sizeof(sizet) bytes of the buffer for props and uncompressed size information
                buf, bufSize,
                outProps, &outPropsSize,
                5, (1 << 20), 3, 0, 2, fb, 1); // 1 << 20: 1MB , 1 << 24: 16 MB

            if (res != SZ_OK)
            {
                printf("Compression failed with code %d.\n", res);
                free(buf);
                free(outBuf);
                continue;
            }
            // After compression destLen is the compressed size of the data

            f = fopen(outputFilename, "wb");
            if (f == NULL)
            {
                printf("Cannot open output file '%s'.\n", outputFilename);
                free(buf);
                free(outBuf);
                continue;
            }

            uncompressedSize = bufSize;                                    // The original file size before compression
            fwrite(&uncompressedSize, sizeof(size_t), 1, f);               // Write the uncompressed size to the start of the file
            fwrite(outProps, 1, outPropsSize, f);                          // Then write the compression properties
            fwrite(outBuf + outPropsSize + sizeof(size_t), 1, destLen, f); // Then write the compressed data

            fclose(f);
            free(outBuf);
        }
        else if (operation == 'd')
        {
            f = fopen(filename, "rb");
            if (f == NULL)
            {
                printf("Cannot open file '%s'.\n", filename);
                continue;
            }

            // Read the uncompressed size from the file
            fread(&uncompressedSize, sizeof(uncompressedSize), 1, f);

            // Read the LZMA properties from the file
            unsigned char props[LZMA_PROPS_SIZE];
            fread(props, LZMA_PROPS_SIZE, 1, f);

            // The remaining file size is the size of the compressed data
            bufSize = bufSize - sizeof(uncompressedSize) - LZMA_PROPS_SIZE;
            unsigned char *compBuf = (unsigned char *)malloc(bufSize);
            if (compBuf == NULL)
            {
                printf("Cannot allocate memory.\n");
                fclose(f);
                continue;
            }

            // Read the compressed data into the buffer
            fread(compBuf, 1, bufSize, f);
            fclose(f);

            // Allocate memory for the decompression buffer
            unsigned char *decompBuf = (unsigned char *)malloc(uncompressedSize);
            if (decompBuf == NULL)
            {
                printf("Cannot allocate memory for the decompression buffer.\n");
                free(compBuf);
                continue;
            }

            size_t srcLen = bufSize;
            res = LzmaUncompress(
                decompBuf, &uncompressedSize, // output
                compBuf, &srcLen,             // input
                props, LZMA_PROPS_SIZE        // properties
            );

            free(compBuf); // Free the compression buffer as it's no longer needed

            if (res != SZ_OK)
            {
                printf("Decompression failed with code %d.\n", res);
                free(decompBuf);
                continue;
            }

            // Write the decompressed data to the output file
            f = fopen(outputFilename, "wb");
            if (f == NULL)
            {
                printf("Cannot open output file '%s'.\n", outputFilename);
                free(decompBuf);
                continue;
            }

            fwrite(decompBuf, 1, uncompressedSize, f);
            fclose(f);
            free(decompBuf);
        }
        else
        {
            printf("Invalid operation. Please enter 'c' to compress or 'd' to decompress.\n");
        }

        free(buf);
        printf("Operation completed successfully, saved as '%s'.\n", outputFilename);
        Sleep(1000);
    }

    return 0;
}