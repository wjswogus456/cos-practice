#ifndef __HUMAN_H__
#define __HUMAN_H__

class Human {
    private:
        const char *name;
        int age;

    public:
        Human(const char *name, int age);
        Human(const char *name);
        const char *getName();
        void setAge(int age);
        int getAge();
        void print();
};

#endif /* __HUMAN_H__ */
