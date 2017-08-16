#include <iostream>

#include <map>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <vector>

class Container {
    struct TypeInfo {
        size_t hash_code;
        size_t offset;
        size_t length;
    };

    friend class ContainerVisitor;

public:
    Container()
    {
        data = nullptr;
        bufferSize = 0;
        size = 0;
    }
    ~Container()
    {
        delete[] data;
    }

    Container(const Container& other) = delete;
    Container(Container&& other) = delete;

    Container& operator=(const Container& other) = delete;
    Container& operator=(Container&& other) = delete;

    template <typename T>
    void push_back(T& obj)
    {
        const size_t objSize = sizeof(obj);

        if (size + objSize >= bufferSize)
            expandContainer(objSize);

        TypeInfo typeInfo;
        typeInfo.hash_code = typeid(obj).hash_code();
        typeInfo.length = objSize;
        typeInfo.offset = 0;
        if (types.size() > 0)
            typeInfo.offset = types.back().offset + types.back().length;

        types.push_back(typeInfo);
        *reinterpret_cast<T*>(&data[typeInfo.offset]) = obj;

        size += objSize;
    }

    // method takes hash_code of type and function for handling this type
    void forEach(std::map<size_t, std::function<void(void*)>> handlers)
    {
        for (const auto& type : types) {
            auto it = handlers.find(type.hash_code);
            if (it != handlers.end())
                it->second(getObjectPointer(type));
        }
    }

    void debug()
    {
        for (size_t i = 0; i < size; ++i) {
            std::cout << (int)*((int*)(&data[i])) << " ";
        }
        std::cout << std::endl;
    }

private:
    char* data;
    std::vector<TypeInfo> types;
    size_t bufferSize;
    size_t size;

    void* getObjectPointer(TypeInfo typeInfo)
    {
        return &data[typeInfo.offset];
    }

    void expandContainer(size_t minimumSize)
    {
        size_t newSize = bufferSize * 2;
        if (newSize < bufferSize + minimumSize)
            newSize += minimumSize;

        char* newData = new char[newSize];

        std::copy(data, data + size, newData);

        delete[] data;
        data = newData;

        bufferSize = newSize;
    }
};

struct Color {
    unsigned char red, green, blue;

    Color(char red, char green, char blue)
        : red(red)
        , green(green)
        , blue(blue)
    {
    }
};

class ContainerVisitor {
public:
    ContainerVisitor(Container& container)
        : container(container)
    {
    }

    void setHandler(const std::type_info& type_info, std::function<void(void*)> handler)
    {
        handlers[type_info.hash_code()] = handler;
    }

    template <typename T>
    void setHandler(std::function<void(void*)> handler)
    {
        handlers[typeid(T).hash_code()] = handler;
    }

    void process()
    {
        container.forEach(handlers);
    }

private:
    Container& container;
    std::map<size_t, std::function<void(void*)>> handlers;
};

int main(int argc, char* argv[])
{
    Container c;

    int ival = 5;
    c.push_back(ival);

    ival = 10;
    c.push_back(ival);

    double dval = 15.46;
    c.push_back(dval);

    Color color(255, 0, 0);
    c.push_back(color);

    ival = 99;
    c.push_back(ival);

    std::cout << std::endl;

    ContainerVisitor visitor(c);

    visitor.setHandler(typeid(int), [](void* obj) {
        int& val = *(int*)obj;

        std::cout << "int: " << val << std::endl;
    });
    visitor.setHandler(typeid(double), [](void* obj) {
        double& val = *(double*)obj;

        std::cout << "double: " << val << std::endl;
    });
    visitor.setHandler(typeid(Color), [](void* obj) {
        Color& val = *(Color*)obj;

        std::cout << "Color: " << int(val.red) << " " << int(val.green) << " " << int(val.blue) << std::endl;
    });

    visitor.process();

    std::cout << "----" << std::endl;

    // with modifying

    visitor.setHandler(typeid(int), [](void* obj) {
        int& val = *(int*)obj;
        val = -1;

        std::cout << "int: " << val << std::endl;
    });

    visitor.process();

    std::cout << "----" << std::endl;

    // nice syntax

    visitor.setHandler<int>([](void* obj) {
        int& val = *(int*)obj;

        std::cout << "It's int type with value " << val << std::endl;
    });

    visitor.process();

    return 0;
}
