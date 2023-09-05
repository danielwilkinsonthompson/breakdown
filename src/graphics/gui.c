/*=============================================================================
                                  gui.c
-------------------------------------------------------------------------------
graphical user interface elements

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "layer.h"
#include "gui.h"
#include "image.h"

void gui_draw_pixel(gui_element *this_element)
{
    *(this_element->parent->render->pixel_data + ((this_element->data->position->y) * this_element->parent->render->width) + (this_element->data->position->x)) = this_element->data->colour;
}

void gui_draw_line(gui_element *this_element)
{
    int32_t x1 = this_element->data->position->x;
    int32_t y1 = this_element->data->position->y;
    int32_t x2 = this_element->data->position->x + this_element->data->position->width;
    int32_t y2 = this_element->data->position->y + this_element->data->position->height;

    // Bresenham's line algorithm
    int32_t dx = abs(x2 - x1);
    int32_t dy = abs(y2 - y1);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;
    for (;;)
    {
        // FIXME: this will wrap around if the line is longer than the render
        *(this_element->parent->render->pixel_data + ((y1)*this_element->parent->render->width) + (x1)) = this_element->data->colour;
        if ((x1 == x2) && (y1 == y2))
            break;
        int32_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void gui_draw_polyline(gui_element *this_element)
{
    int32_t x1, y1, x2, y2;

    for (uint32_t i = 0; i < this_element->data->num_points - 1; i++)
    {
        x1 = this_element->data->position[i].x;
        y1 = this_element->data->position[i].y;
        x2 = this_element->data->position[i + 1].x;
        y2 = this_element->data->position[i + 1].y;

        // Bresenham's line algorithm
        int32_t dx = abs(x2 - x1);
        int32_t dy = abs(y2 - y1);
        int32_t sx = (x1 < x2) ? 1 : -1;
        int32_t sy = (y1 < y2) ? 1 : -1;
        int32_t err = dx - dy;
        for (;;)
        {
            // FIXME: this will wrap around if the line is longer than the render
            *(this_element->parent->render->pixel_data + ((y1)*this_element->parent->render->width) + (x1)) = this_element->data->colour;
            if ((x1 == x2) && (y1 == y2))
                break;
            int32_t e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y1 += sy;
            }
        }
    }
}

void gui_draw_rectangle(gui_element *this_element)
{
    int32_t x1 = this_element->data->position->x;
    int32_t y1 = this_element->data->position->y;
    int32_t x2 = this_element->data->position->x + this_element->data->position->width;
    int32_t y2 = this_element->data->position->y + this_element->data->position->height;

    // top & bottom
    for (uint32_t x = x1; x < x2; x++)
    {
        *(this_element->parent->render->pixel_data + ((y1)*this_element->parent->render->width) + (x)) = this_element->data->colour;
        *(this_element->parent->render->pixel_data + ((y2)*this_element->parent->render->width) + (x)) = this_element->data->colour;
    }
    // right & left
    for (uint32_t y = y1; y < y2; y++)
    {
        *(this_element->parent->render->pixel_data + ((y)*this_element->parent->render->width) + (x1)) = this_element->data->colour;
        *(this_element->parent->render->pixel_data + ((y)*this_element->parent->render->width) + (x2)) = this_element->data->colour;
    }
}

void gui_draw_image(gui_element *this_element)
{
    // should be able to do all this with memcpy
    image *this_image, *render;
    coordinates position;
    uint32_t render_x_start, render_x_stop, render_width;
    uint32_t render_y_start, render_y_stop, render_height;
    uint32_t this_image_x_start, this_image_y_start;

    // check inputs
    if (this_element == NULL)
        return;

    if (this_element->parent == NULL || this_element->data == NULL)
        return;

    if (this_element->parent->render == NULL || this_element->data->img == NULL)
        return;

    if (this_element->data->position->width == 0 || this_element->data->position->height == 0)
        return;

    // convenience labels
    this_image = this_element->data->img;
    render = this_element->parent->render;
    position = *this_element->data->position; // TODO: check whether you can just assign this_element->data->position to position

    if (this_image->width != position.width || this_image->height != position.height)
        this_image = image_resize(this_image, position.height, position.width);

    render_x_start = (position.x <= 0) ? 0 : position.x;
    render_x_stop = (position.x + this_image->width > render->width) ? render->width : position.x + this_image->width;
    render_width = render_x_stop - render_x_start;

    this_image_x_start = (position.x <= 0) ? abs(position.x) : 0;
    this_image_y_start = (position.y <= 0) ? abs(position.y) : 0;

    render_y_start = (position.y <= 0) ? 0 : position.y;
    render_y_stop = (position.y + this_image->height > render->height) ? render->height : position.y + this_image->height;
    render_height = render_y_stop - render_y_start;

    // FIXME: if the image is larger than the render in x, it crashes
    // FIXME: for now, we won't composite images with alpha, just overwrite the pixels
    for (uint32_t row = 0; row < render_height; row++)
        for (uint32_t col = 0; col < render_width; col++)
            *(render->pixel_data + ((render_y_start + row) * render->width) + (render_x_start + col)) = *(this_image->pixel_data + ((this_image_y_start + row) * this_image->width) + this_image_x_start + col);

    if (this_image != this_element->data->img)
    {
        free(this_image->pixel_data);
        free(this_image);
    }
}

gui_element *gui_element_init(layer *parent, gui_element_type type, coordinates position)
{
    gui_element *this_element = (gui_element *)malloc(sizeof(gui_element));
    if (this_element == NULL)
        goto memory_error;
    this_element->type = type;
    this_element->data = (gui_element_data *)malloc(sizeof(gui_element_data));
    if (this_element->data == NULL)
        goto memory_error;
    this_element->data->position = (coordinates *)malloc(sizeof(coordinates));
    if (this_element->data->position == NULL)
        goto memory_error;
    *this_element->data->position = position;
    this_element->parent = parent;

    switch (type)
    {
    case gui_pixel:
        this_element->draw = gui_draw_pixel;
        break;
    case gui_image:
        this_element->draw = gui_draw_image;
        break;
    // case gui_text:
    //     this_element->draw = ui_text_draw;
    //     break;
    case gui_rectangle:
        this_element->draw = gui_draw_rectangle;
        break;
    // case gui_ellipse:
    //     this_element->draw = gui_ellipse_draw;
    //     break;
    case gui_line:
        this_element->draw = gui_draw_line;
        break;
    case gui_polyline:
        this_element->draw = gui_draw_polyline;
        break;
    // case gui_polygon:
    //     this_element->draw = ui_polygon_draw;
    //     break;
    // case gui_sprite:
    //     this_element->draw = ui_sprite_draw;
    //     break;
    // case gui_textbox:
    //     this_element->draw = ui_textbox_draw;
    //     break;
    // case gui_button:
    //     this_element->draw = ui_button_draw;
    //     break;
    // case gui_slider:
    //     this_element->draw = ui_slider_draw;
    //     break;
    // case gui_checkbox:
    //     this_element->draw = ui_checkbox_draw;
    //     break;
    // case gui_radiobutton:
    //     this_element->draw = ui_radiobutton_draw;
    //     break;
    // case gui_dropdown:
    //     this_element->draw = ui_dropdown_draw;
    //     break;
    // case gui_inputbox:
    //     this_element->draw = ui_inputbox_draw;
    //     break;
    // case gui_scrollbar:
    //     this_element->draw = ui_scrollbar_draw;
    //     break;
    // case gui_scrollbox:
    //     this_element->draw = ui_scrollbox_draw;
    //     break;
    // case gui_listbox:
    //     this_element->draw = ui_listbox_draw;
    //     break;
    // case gui_canvas:
    //     this_element->draw = ui_canvas_draw;
    //     break;
    // case gui_plot:
    //     this_element->draw = ui_plot_draw;
    //     break;
    // case gui_table:
    //     this_element->draw = ui_table_draw;
    //     break;
    // case gui_menu:
    //     this_element->draw = ui_menu_draw;
    //     break;
    // case gui_window:
    //     this_element->draw = ui_window_draw;
    //     break;
    // case gui_dialog:
    //     this_element->draw = ui_dialog_draw;
    //     break;
    // case gui_popup:
    //     this_element->draw = ui_popup_draw;
    //     break;
    // case gui_tooltip:
    //     this_element->draw = ui_tooltip_draw;
    //     break;
    // case gui_cursor:
    //     this_element->draw = ui_cursor_draw;
    //     break;
    // case gui_keyboard:
    //     this_element->draw = ui_keyboard_draw;
    //     break;
    // case gui_unspecified:
    //     this_element->draw = ui_unspecified_draw;
    //     break;
    default:
        printf("gui_element_init: unknown gui element type.\n");
        break;
    }

    layer_add_gui_element(parent, this_element);

    return this_element;
memory_error:
    gui_element_free(this_element);
    printf("gui_element_init: memory allocation failed.\n");

    return NULL;
}

void gui_element_print(gui_element *this_element)
{
    // FIXME: need to update with new gui_element_data struct
    printf("->type: %d\r\n", this_element->type);
    printf("->data: gui_element_data @ %p\r\n", this_element->data);
    printf("{\r\n");
    printf(" .colour: 0x%08X\r\n", this_element->data->colour);
    printf(" .text: %s\r\n", this_element->data->text);
    printf(" .position: { x: %d, y: %d, z: %d, width: %d, height: %d}\r\n", this_element->data->position->x, this_element->data->position->y, this_element->data->position->z, this_element->data->position->width, this_element->data->position->height);
    printf(" .num_points: %d\r\n", this_element->data->num_points);
    printf("}\r\n");
    printf("->parent: layer @ %p\r\n", this_element->parent);
    printf("->draw: function @ %p\r\n", this_element->draw);
}

void gui_element_free(gui_element *this_element)
{
    // FIXME: need to update with new gui_element_data struct
    if (this_element == NULL)
        return;
    if (this_element->parent != NULL)
        layer_remove_gui_element(this_element);
    if (this_element->data != NULL)
    {
        if (this_element->data->img != NULL)
            image_free(this_element->data->img);
        if (this_element->data->position != NULL)
            free(this_element->data->position);
        if (this_element->data->text != NULL)
            free(this_element->data->text);
        free(this_element->data); // FIXME: this might need to call specific free functions for each gui element type
    }
    free(this_element);
}