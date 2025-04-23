#pragma once
#include <list>
#include "Album.h"
#include "User.h"
#include "IDataAccess.h"

class MemoryAccess : public IDataAccess
{

public:
	MemoryAccess() = default;
	virtual ~MemoryAccess() = default;

	// album related
	const std::list<Album> getAlbums() override;
	const std::list<Album> getAlbumsOfUser(const User& user) override;
	void deleteAlbumsOfUser(const int id);
	void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) const override;
	Album openAlbum(const std::string& albumName, const int userId) override;
	void closeAlbum(Album &pAlbum) override;
	void printAlbums() override;

	// picture related
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture, const int ownerId) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName, const int ownerId) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserFromAllPictures(const User& user);

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
	void close() override {};
	void clear() override;

private:
	std::list<Album> m_albums;
	std::list<User> m_users;

	auto getAlbumIfExists(const std::string& albumName);

	Album createDummyAlbum(const User& user);
	void cleanUserData(const User& userId);
};
