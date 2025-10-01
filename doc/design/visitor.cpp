#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Basic class hierarchy
// ----------------------------------------------------------------------------

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

    // All Nodes must be able to _accept_ a visitor
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

    void accept(Visitor *visitor) const override
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

    void accept(Visitor *visitor) const override
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

// ----------------------------------------------------------------------------
// Path visitor
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Test harness
// ----------------------------------------------------------------------------

int main()
{
    auto a = std::make_shared<Directory>("a");
    a->children().emplace_back(std::make_shared<File>("c", 300));
    a->children().emplace_back(std::make_shared<File>("d", 20));

    auto b = std::make_shared<Directory>("b");
    b->children().emplace_back(std::make_shared<File>("e", 256));
    b->children().emplace_back(std::make_shared<File>("f", 1000));

    auto root = std::make_shared<Directory>("root");
    root->children().push_back(a);
    root->children().push_back(b);

    PathVisitor visitor;
    root->accept(&visitor);
}
