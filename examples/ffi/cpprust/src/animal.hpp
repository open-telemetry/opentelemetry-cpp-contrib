class Animal {
    public:
        Animal(int age) : value(age) {}
        int get_age() const {return value;}

    private:
        int value;
};