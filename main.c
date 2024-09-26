#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <pthread.h>
#include <time.h>

#include <windows.h>
#include <conio.h>

void gotoxy(int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

#define WIDTH 20
#define HEIGHT 10
#define FPS 30

static int running = 1;
static pthread_mutex_t view_mutex;

// Linked List

typedef struct node_t
{
    void *data;
    struct node_t *next;
    struct node_t *prev;
} node_t;

typedef struct linked_list_t
{
    node_t *head;
    node_t *tail;
    int size;

    void (*add)(struct linked_list_t *self, void *data);
    void (*insert)(struct linked_list_t *self, void *data, int index);
    void (*remove)(struct linked_list_t *self, void *data);
    void (*clear)(struct linked_list_t *self);
} linked_list_t;

void add_node(linked_list_t *self, void *data)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->data = data;
    node->next = NULL;
    node->prev = self->tail;

    if (self->head == NULL)
    {
        self->head = node;
    }
    else
    {
        self->tail->next = node;
    }

    self->tail = node;
    self->size++;
}

void insert_node(linked_list_t *self, void *data, int index)
{
    if (index < 0 || index > self->size)
    {
        return;
    }

    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->data = data;

    if (index == 0)
    {
        node->next = self->head;
        node->prev = NULL;

        if (self->head != NULL)
        {
            self->head->prev = node;
        }

        self->head = node;

        if (self->tail == NULL)
        {
            self->tail = node;
        }
    }
    else if (index == self->size)
    {
        node->next = NULL;
        node->prev = self->tail;

        if (self->tail != NULL)
        {
            self->tail->next = node;
        }

        self->tail = node;

        if (self->head == NULL)
        {
            self->head = node;
        }
    }
    else
    {
        node_t *current = self->head;
        for (int i = 0; i < index - 1; i++)
        {
            current = current->next;
        }

        node->next = current->next;
        node->prev = current;
        current->next->prev = node;
        current->next = node;
    }

    self->size++;
}

void remove_node(linked_list_t *self, void *data)
{
    node_t *current = self->head;
    node_t *prev = NULL;

    while (current != NULL)
    {
        if (current->data == data)
        {
            if (prev == NULL)
            {
                self->head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            if (current->next == NULL)
            {
                self->tail = prev;
            }
            else
            {
                current->next->prev = prev;
            }

            free(current);
            self->size--;
            break;
        }

        prev = current;
        current = current->next;
    }
}

void clear_node(linked_list_t *self)
{
    node_t *current = self->head;
    node_t *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
}

void init_linked_list(linked_list_t *linked_list)
{
    linked_list->head = NULL;
    linked_list->tail = NULL;
    linked_list->size = 0;

    linked_list->add = &add_node;
    linked_list->insert = &insert_node;
    linked_list->remove = &remove_node;
    linked_list->clear = &clear_node;
}

linked_list_t create_linked_list()
{
    linked_list_t linked_list;
    init_linked_list(&linked_list);
    return linked_list;
}

// Linked List end

// Listenable

typedef struct listenable_t
{
    void (**listeners)(void *self);
    int length;
    int count;
    const char *key;
    void *value;

    void (*add_listener)(struct listenable_t *self, void (*listener)());
    void (*notify_listeners)(struct listenable_t *self);
    void (*remove_listener)(struct listenable_t *self, void (*listener)());
    void (*set_value)(struct listenable_t *self, void *value);

} listenable_t;

void add_listener(listenable_t *self, void (*listener)())
{
    if (self->count == self->length)
    {
        if (self->count == 0)
        {
            self->listeners = (void (**)(void *))malloc(sizeof(void (*)(void *)));
            self->length = 1;
        }
        else
        {
            self->listeners = (void (**)(void *))realloc(self->listeners, sizeof(void (*)(void *)) * self->length * 2);
            self->length *= 2;
        }
    }
    *(self->listeners + self->count) = listener;
    self->count++;
}

void notify_listeners(listenable_t *self)
{
    for (int i = 0; i < self->count; i++)
    {
        (*(self->listeners + i))(self);
    }
}

void remove_listener(listenable_t *self, void (*listener)())
{
    for (int i = 0; i < self->count; i++)
    {
        if (*(self->listeners + i) == listener)
        {
            for (int j = i; j < self->count - 1; j++)
            {
                *(self->listeners + j) = *(self->listeners + j + 1);
            }
            self->count--;
        }
    }

    if (self->count < self->length / 2)
    {
        self->listeners = (void (**)(void *))realloc(self->listeners, sizeof(void (*)(void *)) * self->length / 2);
        self->length /= 2;
    }
}

void set_value(listenable_t *self, void *value)
{
    self->value = value;
    self->notify_listeners(self);
}

void init_listenable(listenable_t *listenable, const char *key, void *value)
{

    listenable->listeners = NULL;
    listenable->length = 0;
    listenable->count = 0;
    listenable->add_listener = &add_listener;
    listenable->notify_listeners = &notify_listeners;
    listenable->remove_listener = &remove_listener;
    listenable->set_value = &set_value;

    listenable->key = key;
    listenable->value = value;
}

listenable_t create_listenable(const char *key, void *value)
{
    listenable_t listenable;
    init_listenable(&listenable, key, value);
    return listenable;
}

void destroy_listenable(listenable_t *listenable)
{

    free(listenable->listeners);
}

// Listenable end

typedef struct point2d_t
{
    int x;
    int y;
} point2d_t;

typedef struct app_t
{
    listenable_t keyboard;
    listenable_t render;
    listenable_t snake;

} app_t;

// Keyboard

void *keyboard_thread(void *arg)
{
    char key;
    listenable_t *listenable = (listenable_t *)arg;

    while (running)
    {
        key = getch();
        listenable->set_value(listenable, &key);
        Sleep(0);
    }
    return NULL;
}

void keyboard_listener(void *arg)
{

    app_t *app = (app_t *)arg;
    char key = *(char *)app->keyboard.value;
    point2d_t *snake = (point2d_t *)app->snake.value;

    pthread_mutex_lock(&view_mutex);
    gotoxy(0, HEIGHT);
    printf("Key pressed: %c\n", key);
    pthread_mutex_unlock(&view_mutex);

    switch (key)
    {
    case 'q':
        running = 0;
        break;
    case 'a':
        snake->x--;
        app->snake.notify_listeners(&app->snake);
        break;
    case 'd':
        snake->x++;
        app->snake.notify_listeners(&app->snake);
        break;
    case 'w':
        snake->y--;
        app->snake.notify_listeners(&app->snake);
        break;
    case 's':
        snake->y++;
        app->snake.notify_listeners(&app->snake);
        break;
    }
}

// Keyboard end

// Render

void *render_thread(void *arg)
{
    listenable_t *listenable = (listenable_t *)arg;

    clock_t now, last = 0;
    double frame_time = 1.0 / FPS;
    double elapsed_time;

    while (running)
    {
        now = clock();

        elapsed_time = ((double)(now - last)) / CLOCKS_PER_SEC;

        if (elapsed_time >= frame_time)
        {
            listenable->set_value(listenable, &elapsed_time);

            last = now;
        }
        Sleep(0);
    }
    return NULL;
}

void draw(app_t *app)
{
    gotoxy(0, 0);

    point2d_t *snake = (point2d_t *)app->snake.value;

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i == 0 || i == HEIGHT - 1)
            {
                printf("-");
            }
            else if (j == 0 || j == WIDTH - 1)
            {
                printf("|");
            }
            else if (j == snake->x && i == snake->y)
            {
                printf("*");
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
    }
}

void render_listener(void *arg)
{
    app_t *app = (app_t *)arg;

    pthread_mutex_lock(&view_mutex);
    draw(app);
    pthread_mutex_unlock(&view_mutex);
}

// Render end

void clear_console()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int main()
{
    clear_console();

    app_t app;

    // Keyboard
    pthread_t keyboard_thread_id;
    app.keyboard = create_listenable("keyboard", NULL);
    app.keyboard.add_listener(&app.keyboard, &keyboard_listener);
    pthread_create(&keyboard_thread_id, NULL, keyboard_thread, &app);

    // Render
    pthread_t render_thread_id;
    app.render = create_listenable("render", NULL);
    app.render.add_listener(&app.keyboard, &render_listener);
    pthread_create(&render_thread_id, NULL, render_thread, &app);

    // Snake
    point2d_t snake = {WIDTH / 2, HEIGHT / 2};
    app.snake = create_listenable("snake", &snake);

    pthread_join(keyboard_thread_id, NULL);
    pthread_join(render_thread_id, NULL);

    clear_console();

    destroy_listenable(&app.keyboard);
    destroy_listenable(&app.render);

    return 0;
}
