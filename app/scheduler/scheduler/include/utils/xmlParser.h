 /*@file
 * @author  Umar Waqas <u.waqas@tue.nl>
 * @version 0.1
 *
 * @section LICENSE
 * The licensing of this software is in progress.
 *
 * @section DESCRIPTION
 * xmlParser is the base class for xml parsing.
 *
 */

#ifndef XML_PARSER_H_
#define XML_PARSER_H_

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>

/* @section DESCRIPTION
* class xmlParser implements basic xml parsing.
*/
class xmlParser{

public:
    explicit xmlParser(std::string fname);
    xmlParser(const xmlParser &) = delete;
    xmlParser(xmlParser &&) = default;
    virtual ~xmlParser() = default;

    xmlParser &operator=(const xmlParser &) = delete;
    xmlParser &operator=(xmlParser &&) = default;

    virtual void loadXml();
    [[nodiscard]] bool isLoaded() const { return m_loaded; }
    [[nodiscard]] rapidxml::xml_node<> *getFirstNode(const char *name) {
        return m_doc->first_node(name);
    }
    friend std::ostream& operator<<(std::ostream &output, const xmlParser &p);

    std::string_view getFileName() { return m_filename; }

private:
    std::string m_filename;
    std::unique_ptr<rapidxml::xml_document<>> m_doc;
    bool m_loaded;
    std::string m_xml;
};
#endif /* XML_PARSER_H_ */
