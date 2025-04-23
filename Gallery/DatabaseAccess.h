#pragma once
#include "IDataAccess.h"
#include "sqlite3.h"
#include <array>
#include <variant>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

constexpr const char* DB_NAME = "Gallery.db";
constexpr const char* ENABLE_FOREIGN_KEY = "PRAGMA foreign_keys = ON;";
constexpr int FILE_EXISTS = 0;
typedef std::vector<std::map<std::string, std::variant<int, std::string>>> selected;

class DatabaseAccess : public IDataAccess
{
public:
	DatabaseAccess() = default;
	virtual ~DatabaseAccess() = default;

	// album related
	const std::list<Album> getAlbums() override;
	const std::list<Album> getAlbumsOfUser(const User& user) override;
	void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) const override;
	Album openAlbum(const std::string& albumName, const int userId) override;
	void closeAlbum(Album& pAlbum) override;
	void printAlbums() override;

	// picture related
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture, const int ownerId) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName, const int ownerId) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	// user related
	void printUsers() override;
	int createUser(User& user) override;
	void deleteUser(const User& user) override;
	bool doesUserExists(int userId) const override;
	User getUser(int userId) override;

	// user statistics
	int countAlbumsOwnedOfUser(const User& user) override;
	int countAlbumsTaggedOfUser(const User& user) override;
	int countTagsOfUser(const User& user) override;
	float averageTagsPerAlbumOfUser(const User& user) override;

	// queries
	User getTopTaggedUser() override;
	Picture getTopTaggedPicture() override;
	std::list<Picture> getTaggedPicturesOfUser(const User& user) override;

	bool open() override;
	void close() override;
	void clear() override;
private: 
	bool defineDb() const;
	void startTransaction() const;
	void commitTransaction() const;
	static void checkSqlInjection(std::string stringToCheck);
	void addTags(const Picture& pic, const std::string& pictureId) const;
	Picture getPictureById(const int picId);
	void removeTags(const int pictureId);
	static int callback(void* data, int argc, char** argv, char** azColName);
	sqlite3* _db;
};