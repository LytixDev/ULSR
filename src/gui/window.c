/*
 *  Copyright (C) 2023 Nicolai Brand, Callum Gran
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stb/stb_image.h>

#include "gui/window.h"
#include "lib/common.h"
#include "ulsr/impl.h"

static i16 node_find(int x, int y)
{
    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
        struct simulation_coord_t *coord = &coords[i];
        if (coord->x < x + 10 && coord->x > x - 10 && coord->y < y + 10 && coord->y > y - 10) {
            return i;
        }
    }
    return -1;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    i16 node_idx = -1;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
	double x_pos;
	double y_pos;
	glfwGetCursorPos(window, &x_pos, &y_pos);
	struct window_data_t *window_data =
	    (struct window_data_t *)glfwGetWindowUserPointer(window);
	if (y_pos <= TOOLBAR_HEIGHT) {
	    int radio_button_width = SIMULATION_WIDTH / TOOLBAR_ITEM_COUNT;

	    int clicked_idx = x_pos / radio_button_width;

	    window_data->selected_radio_button = clicked_idx;
	} else if ((node_idx = node_find(x_pos, SIMULATION_LENGTH - y_pos)) != -1) {
        if (window_data->selected_node == node_idx) {
            window_data->selected_node = -1;
        } else {
            window_data->selected_node = node_idx;
        }
    } else {
        if (window_data->selected_node != -1) {            
            update_coord(window_data->selected_node + 1, (int) x_pos, (int)(SIMULATION_LENGTH - y_pos));
        }
    }
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0.0f, 0.0f, width, height);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(0, width, 0, height, 0, 1);

    glMatrixMode(GL_MODELVIEW);
}

GLFWwindow *window_create()
{
    GLFWwindow *window;

    if (!glfwInit()) {
	fprintf(stderr, "Failed to initialize window.\n");
	return NULL;
    }

    window = glfwCreateWindow(SIMULATION_WIDTH, SIMULATION_LENGTH, "ULSR Simulation", NULL, NULL);

    GLFWimage images[1];
    images[0].pixels =
	stbi_load("./include/static/icon.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);

    if (!window) {
	glfwTerminate();
	return NULL;
    }

    int width = 0;
    int height = 0;

    glfwSetWindowSizeLimits(window, SIMULATION_WIDTH, SIMULATION_LENGTH, SIMULATION_WIDTH,
			    SIMULATION_LENGTH);
    glfwSetWindowAspectRatio(window, SIMULATION_WIDTH, SIMULATION_LENGTH);
    glfwSetWindowAspectRatio(window, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwGetFramebufferSize(window, &width, &height);

    glfwMakeContextCurrent(window);

    GLenum glewInitStatus = glewInit();

    glViewport(0.0f, 0.0f, width, height);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);

    glMatrixMode(GL_MODELVIEW);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    struct window_data_t *window_data = malloc(sizeof(struct window_data_t));
    window_data->selected_radio_button = 0;
    window_data->selected_node = -1;
    window_data->arrow_queue = malloc(sizeof(struct queue_t));
    queue_init(window_data->arrow_queue, 32);

    glfwSetWindowUserPointer(window, (void *)window_data);

    return window;
}

static void draw_circle(float center_x, float center_y, float radius)
{
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINE_LOOP);
    u16 i;
    for (i = 0; i < 360; i++) {
	float theta = i * 0.017;
	float x = center_x + radius * cos(theta);
	float y = center_y + radius * sin(theta);
	glVertex2f(x, y);
    }
    glEnd();
}

void draw_arrow(float x1, float y1, float x2, float y2, int is_send) {
    if (is_send) {
        glColor3f(0, 255, 255);
        x1 -= 5;
        x2 -= 5;
    } else {
        glColor3f(255, 255, 0);
        x1 += 5;
        x2 += 5;
    }

    GLfloat line_vertices[] = { x1, y1, x2, y2 };
    
    GLfloat dx = x2 - x1;
    GLfloat dy = y2 - y1;
    GLfloat length = sqrt(dx * dx + dy * dy);
    dx /= length;
    dy /= length;

    GLfloat arrowhead_length = 10;
    GLfloat arrowhead_angle = 1;
    GLfloat arrowhead_vertices[] = {
        x2, y2,
        x2 - arrowhead_length * (dx * cos(arrowhead_angle) + dy * sin(arrowhead_angle)),
        y2 - arrowhead_length * (dy * cos(arrowhead_angle) - dx * sin(arrowhead_angle)),
        x2 - arrowhead_length * (dx * cos(arrowhead_angle) - dy * sin(arrowhead_angle)),
        y2 - arrowhead_length * (dy * cos(arrowhead_angle) + dx * sin(arrowhead_angle))
    };

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(10);
    glEnableClientState(GL_VERTEX_ARRAY);
        
    glVertexPointer(2, GL_FLOAT, 0, line_vertices);
    glDrawArrays(GL_LINES, 0, 2);

    glVertexPointer(2, GL_FLOAT, 0, arrowhead_vertices);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_LINE_SMOOTH);
}


static void draw_node_coords(u16 selected_node)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glPointSize(20);
    glEnable(GL_POINT);

    GLfloat colors[MESH_NODE_COUNT * 3];
    GLfloat point_vertices[MESH_NODE_COUNT * 2];

    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
        if (i == selected_node) {
            colors[i * 3] = 255;
            colors[i * 3 + 1] = 0;
            colors[i * 3 + 2] = 255;
        } else {
            colors[i * 3] = 0;
            colors[i * 3 + 1] = 255;
            colors[i * 3 + 2] = 0;
        }
        point_vertices[i * 2] = coords[i].x;
        point_vertices[i * 2 + 1] = coords[i].y;
    }

    glColorPointer(3, GL_FLOAT, 0, colors);
    glVertexPointer(2, GL_FLOAT, 0, point_vertices);
    glDrawArrays(GL_POINTS, 0, MESH_NODE_COUNT);

    glDisable(GL_POINT);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_target_coords()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_POINT);
    glEnableClientState(GL_COLOR_ARRAY);
    glPointSize(50);

    GLfloat point_vertex[] = { target_coords.x, target_coords.y, 0 };

    GLfloat colors[] = { 255, 0, 0, 255, 0, 0, 255, 0, 0 };

    glColorPointer(3, GL_FLOAT, 0, colors);
    glVertexPointer(2, GL_FLOAT, 0, point_vertex);
    glDrawArrays(GL_POINTS, 0, 1);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_POINT);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_ranges()
{
    float current_time = glfwGetTime();
    float radius = STARTING_RING_RADIUS + (RING_SPEED * current_time);

    if (radius > SIMULATION_NODE_RANGE) {
	glfwSetTime(0);
    }

    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
	draw_circle(coords[i].x, coords[i].y, radius);
    }
}

static void draw_toolbar(int selected_radio_button)
{
    init_free_type();
    load_font();
    u16 radio_button_width = SIMULATION_WIDTH / TOOLBAR_ITEM_COUNT;
    u8 i;
    for (i = 0; i < TOOLBAR_ITEM_COUNT; i++) {

	int x = i * radio_button_width;
	int y = SIMULATION_LENGTH - TOOLBAR_HEIGHT;
	int radio_button_height = TOOLBAR_HEIGHT;

	if (i == selected_radio_button) {
	    glColor3f(1.0f, 0.0f, 1.0f);
	} else {
	    glColor3f(0.5f, 0.5f, 0.5f); 
	}

	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + radio_button_width, y);
	glVertex2f(x + radio_button_width, y + radio_button_height);
	glVertex2f(x, y + radio_button_height);
	glEnd();
	render_text(toolbar_items[i],
		    x + ((radio_button_width - (strlen(toolbar_items[i]) * 4)) / 5),
		    y + (TOOLBAR_HEIGHT - 12) / 2, 1.0f);
    }
}

static void draw_arrows(struct queue_t *arrows)
{
    for (int i = arrows->start; i != arrows->end; i = (i + 1) % arrows->max) {
        struct arrow_queue_data_t *data = (struct arrow_queue_data_t *)arrows->items[i];
        draw_arrow(coords[data->from_node - 1].x,
            coords[data->from_node - 1].y,
            coords[data->to_node - 1].x,
            coords[data->to_node - 1].y,
            data->is_send);
    }
}

void window_update(GLFWwindow *window)
{
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    struct window_data_t *window_data = (struct window_data_t *)glfwGetWindowUserPointer(window);

    glClear(GL_COLOR_BUFFER_BIT);

    draw_node_coords(window_data->selected_node);

    draw_target_coords();

    draw_ranges();

    draw_toolbar(window_data->selected_radio_button);

    draw_arrows(window_data->arrow_queue);

    glfwSwapBuffers(window);

    glfwPollEvents();
}

void window_destroy(GLFWwindow *window)
{
    clean_font();
    glfwDestroyWindow(window);
    glfwTerminate();
}