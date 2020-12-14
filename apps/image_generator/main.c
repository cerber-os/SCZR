#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct pixel {
    char red;
    char green;
    char blue;
};

struct pixel** generate_image(int width, int height)
{
    struct pixel **image;
    image = malloc(height*sizeof(struct pixel*));
    for(int i=0; i<height; i++)
    {
        image[i] = (struct pixel*)malloc(width*sizeof(struct pixel));
    }
    srand(time(0));
    // we need to fill first row with random colors
    for(int i=0; i<width; i++)
    {
        image[0][i].red = rand()%255;
        image[0][i].green = rand()%255;
        image[0][i].blue = rand()%255;
    }
    // we fill the rest of rows
    for(int i=1; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            image[i][j].red = image[0][(i+j)%width].red;
            image[i][j].green = image[0][(i+j)%width].green;
            image[i][j].blue = image[0][(i+j)%width].blue;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int width, height;
    struct pixel** image;
    if(argc<3)
    {
        printf("Not enough arguments. We will use default width and height\n");
        width = height = 100;
    }
    else
    {
        width = atoi(argv[1]);
        height = atoi(argv[1]);
    }  
    image = generate_image(width, height);
    return 0;
}