#include "ft.h"

int face_get_line_spacing(FT_Face face)
{
    return face->size->metrics.height / 64;
}

void face_get_char_extent(FT_Face face, int c, int *x, int *y)
{
    FT_UInt gi = FT_Get_Char_Index(face, c);
    FT_Load_Glyph (face, gi, FT_LOAD_NO_BITMAP);
    *y = face_get_line_spacing (face);
    *x = face->glyph->metrics.horiAdvance / 64;
}

void face_get_string_extent(FT_Face face, const int32_t *s, int *x, int *y)
{
    *x = 0;
    int y_extent = 0;
    while (*s)
    {
        int x_extent;
        face_get_char_extent(face, *s, &x_extent, &y_extent);
        *x += x_extent;
        s++;
    }
    *y = y_extent;
}

int next_utf8_glyph_length(const uint8_t *word) {
    // 1-byte glyph (0xxxxxxx).
    if ((*word & 0x80) == 0) {
        return 1;
    }
    // 2-byte glyph (110xxxxx).
    else if ((*word & 0xE0) == 0xC0) {
        return 2;
    }
    // 3-byte glyph (1110xxxx).
    else if ((*word & 0xF0) == 0xE0) {
        return 3;
    }
    // 4-byte glyph (11110xxx).
    else if ((*word & 0xF8) == 0xF0) {
        return 4;
    }

    // Invalid UTF-8 glyph.
    return -1;
}

int32_t *utf8_to_utf32(const uint8_t *utf8_word)
{
    // Compute the length of the resulting UTF-32 sequence.
    int word_length = 0;
    const uint8_t *utf8_word_ptr = utf8_word;

    while (*utf8_word_ptr) {
        int curr_glyph_length = next_utf8_glyph_length(utf8_word_ptr);
        utf8_word_ptr += curr_glyph_length;
        word_length++;
    }

    // Allocate memory for the UTF-32 sequence.
    int32_t *utf32_word = (int32_t *)malloc((word_length + 1) * sizeof(int32_t));
    int32_t *utf32_word_ptr = utf32_word;
    utf8_word_ptr = utf8_word;

    // Convert UTF-8 to UTF-32 sequence.
    while (*utf8_word_ptr) {
        int curr_glyph_length = next_utf8_glyph_length(utf8_word_ptr);
        if (curr_glyph_length == 1) {
            *utf32_word_ptr = *utf8_word_ptr;
        }
        else if (curr_glyph_length == 2) {
            *utf32_word_ptr = ((*utf8_word_ptr & 0x1F) << 6);
            *utf32_word_ptr |= (*(utf8_word_ptr + 1) & 0x3F);
        }
        else if (curr_glyph_length == 3) {
            *utf32_word_ptr = ((*utf8_word_ptr & 0x0F) << 12);
            *utf32_word_ptr |= ((*(utf8_word_ptr + 1) & 0x3F) << 6);
            *utf32_word_ptr |= (*(utf8_word_ptr + 2) & 0x3F);
        }
        else if (curr_glyph_length == 4) {
            *utf32_word_ptr = ((*utf8_word_ptr & 0x07) << 18);
            *utf32_word_ptr |= ((*(utf8_word_ptr + 1) & 0x3F) << 12);
            *utf32_word_ptr |= ((*(utf8_word_ptr + 2) & 0x3F) << 6);
            *utf32_word_ptr |= (*(utf8_word_ptr + 3) & 0x3F);
        }
        utf32_word_ptr++;

        // Prepare for processing the next glyph.
        utf8_word_ptr += curr_glyph_length;
    }

    // Null-terminate the UTF-32 sequence.
    *utf32_word_ptr = 0;

    return utf32_word;
}

void ft_draw_char(FT_Face face, FrameBuffer *fb, int c, size_t *x, size_t y) {
    FT_UInt gi = FT_Get_Char_Index (face, c);
    FT_Load_Glyph (face, gi, FT_LOAD_DEFAULT);
    int bbox_ymax = face->bbox.yMax / 64;
    int y_off = bbox_ymax - face->glyph->metrics.horiBearingY / 64;
    int glyph_width = face->glyph->metrics.width / 64;
    int advance = face->glyph->metrics.horiAdvance / 64;
    int x_off = (advance - glyph_width) / 2;
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    for (int i = 0; i < (int)face->glyph->bitmap.rows; i++)
    {
        int row_offset = y + i + y_off;
        for (int j = 0; j < (int)face->glyph->bitmap.width; j++)
        {
            unsigned char p = face->glyph->bitmap.buffer [i * face->glyph->bitmap.pitch + j];
            if (p)
            {
                fb_set_pixel(fb, *x + j + x_off, row_offset, rgb_to_rgb565(p, p, p));
            }
        }
    }
    *x += advance;
}

void ft_draw_string(FT_Face face, FrameBuffer *fb, const uint8_t *s, size_t x, size_t y)
{
    int32_t *s32 = utf8_to_utf32(s);

    while (*s32)
    {
        ft_draw_char(face, fb, *s32, &x, y);
        s32++;
    }
}

bool ft_init(const char* ttf_file, FT_Face* face, FT_Library* ft, int req_size)
{
    bool ret = false;
    printf("Requested glyph size is %d px\n", req_size);
    if (FT_Init_FreeType(ft) == 0)
    {
        if (FT_New_Face(*ft, ttf_file, 0, face) == 0)
        {
            // Note -- req_size is a request, not an instruction
            if (FT_Set_Pixel_Sizes(*face, 0, req_size) == 0)
            {
                ret = true;
            }
            else
            {
                perror("Can't set font size");
                return false;
            }
        }
        else
        {
            perror("Can't load TTF file");
            return false;
        }
    }
    else
    {
        perror("Can't initialize FreeType library");
        return false;
    }

    return ret;
}