#include "app/app.hpp"

int main(int argc, char** argv)
{
    ash::app::App app(argc, argv);
    return app.run();
}