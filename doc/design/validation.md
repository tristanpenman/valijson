# Validation

## Visitor Pattern

The Visitor Pattern is a design pattern that allows you to separate an algorithm from the objects or data structures on which it operates.

Many descriptions of the Visitor Pattern use shapes (e.g. circles, squares, triangles) as a motivating example. I'll use a file system here, since the hierarchical structure is relevant to schema validation.

The full code for this example can be found in [visitor.cpp](visitor.cpp).

We begin with a basic Visitor class hierarchy:

```c++

class File;
class Directory;

// Base class for all visitors
class Visitor
{
public:
    // Visitors must be able to visit File and Directory objects
    virtual void visit(const File* file) = 0;
    virtual void visit(const Directory* dir) = 0;
};

// Both File and Directory are a kind of Node
class Node
{
public:
    virtual ~Node() = default;

    // Every node must have a name
    Node(const std::string &name)
      : _name(name) {}

    // All Nodes must also be able to _accept_ a visitor
    virtual void accept(Visitor* visitor) const = 0;

    std::string name() const
    {
        return _name;
    }

private:
    std::string _name;
};

// Files have names, like all nodes, and also a file size
class File: public Node
{
public:
    File(const std::string &name, int size)
      : Node(name)
      , _size(size) {}

    void accept(Visitor *visitor) override
    {
        visitor->visit(this);
    }

    int size() const
    {
        return _size;
    }

private:
    int _size;
};

class Directory: public Node
{
public:
    typedef std::vector<std::shared_ptr<Node>> Nodes;

    // Directories have a name, but _not_ a size
    Directory(const std::string &name)
      : Node(name) {}

    void accept(Visitor *visitor) override
    {
        visitor->visit(this);
    }

    // Directories also have children
    Nodes& children()
    {
        return _children;
    }

    // Our Visitors cannot modify Nodes, so we need
    // a const version of this getter
    const Nodes& children() const
    {
        return _children;
    }

private:
    Nodes _children;
};
```

The first part of this solution is the `Visitor` base class, which can be used to visit a `File` or a `Directory`, using function overloading to choose which variation of the `visit()` method to invoke.

The second part of this solution is the `Node` base class, which defines an `accept()` method, taking a `Visitor` as its first and only argument. Note that all `Node` types must have a name, as per the base class. However only `Directory` nodes have children.

Both `File` and `Directory` must provide their own implementations of `accept()`. The purpose of these different versions of `accept()` is to choose the correct version of `visit()` to be invoked, for a given kind of `Node`.

To make this more concrete, we can build a simple `Visitor` implementation. The purpose of this visitor is to print the full path of each file or directory in the hierarchy. Files will be suffixed with their file size.

Here's the code:

```c++
class PathVisitor: public Visitor
{
public:
    virtual void visit(const File* file)
    {
        // Files do not have children, just print the full path
        std::cout << _prefix << file->name();

        // Also print the file size
        std::cout << " (" << file->size() << ")" << std::endl;
    }

    virtual void visit(const Directory* dir)
    {
        // Start by printing the path
        std::cout << _prefix << dir->name() << std::endl;

        // Update prefix before traversing child nodes
        auto oldPrefix = _prefix;
        _prefix = _prefix + dir->name() + "/";

        // Traverse children
        for (auto node : dir->children()) {
            node->accept(this);
        }

        // Restore prefix before returning to parent
        _prefix = oldPrefix;
    }

private:
    std::string _prefix;
};
```

Now we can set up a test harness:

```c++
int main()
{
    auto a = std::make_shared<Directory>("a");
    a->children().emplace_back(std::make_shared<File>("1", 300));
    a->children().emplace_back(std::make_shared<File>("2", 20));

    auto b = std::make_shared<Directory>("b");
    b->children().emplace_back(std::make_shared<File>("3", 256));
    b->children().emplace_back(std::make_shared<File>("4", 1000));

    auto root = std::make_shared<Directory>("root");
    root->children().push_back(a);
    root->children().push_back(b);

    PathVisitor visitor;
    root->accept(visitor);
}
```

The example can be compiled with `g++ visitor.cpp` (or similar for clang). The output of running the program will look like this:

```
% ./a.out
root
root/a
root/a/c (300)
root/a/d (20)
root/b
root/b/e (256)
root/b/f (1000)
```

### Other Examples

It's not hard to think of other visitors that could be implemented for our `File` and `Directory` nodes. One example could be a file size calculator, that finds the total file size for a hierarchy. Another could be a file locator, which finds the list of paths matching a regex.

### Pros

One advantage of the Visitor pattern is that it follows the [Open/Closed principle](https://en.wikipedia.org/wiki/Open%E2%80%93closed_principle) - that classes should be open for extension, but closed for modification. In this example, we can use the Visitor base class to implement new operations on our files / directories with making any further changes to the `File` or `Directory` classes.

The Visitor pattern also groups related behaviors. Each Visitor implementation is fully encapsulated, and does not need to know about other Visitors.

Finally, we've seen that this pattern works well for generic operations applied to a hierarchy. This characteristic makes it distinct from the _container_/_algorithm_/_iterator_ concepts found in the STL.

### Cons

There are, of course, some disadvantages.

It can be harder to add new element types, especially when there are multiple Visitor implementations that need to be updated. When you add a new element type, you have to implement an overload of `visit()` for each and every Visitor implementation. This may be a nuisance in a smaller code base, or a considerable burden in larger, distributed code bases.

The Visitor pattern can also requires breaking encapsulation to some extent (e.g. if the visitor might need access to internals that would otherwise be excluded from a public interface). It's not uncommon to see C++ `friend` declarations being used as a workaround for this.

## Schema Validation

You have a stable class hierarchy and need to perform many unrelated operations.

You want to keep operations decoupled from the objects they operate on.

## Validation Errors

TODO: How are validation errors reported?
