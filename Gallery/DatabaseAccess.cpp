#include "DatabaseAccess.h"
#include "io.h"

User DatabaseAccess::getTopTaggedUser()
{
	//get most common id in tags table
	std::string query = "SELECT USER_ID FROM TAGS GROUP BY USER_ID ORDER BY COUNT(USER_ID) DESC LIMIT 1;";
	selected mostCommon;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, query.c_str(), callback, &mostCommon, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("faild to get most common user id");
	}
	if (mostCommon.empty())
	{
		throw std::exception("no tags");
	}
	return this->getUser(std::get<int>(mostCommon[0]["USER_ID"]));
}

Picture DatabaseAccess::getTopTaggedPicture()
{
	//get most common picture id in tags
	std::string query = "SELECT PICTURE_ID FROM TAGS GROUP BY PICTURE_ID ORDER BY COUNT(PICTURE_ID) DESC LIMIT 1;";
	selected returnedData;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, query.c_str(), callback, &returnedData, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("failed to get most common picture id");
	}
	if (returnedData.empty())
	{
		throw std::exception("no tags");
	}
	int picId = std::get<int>(returnedData[0]["PICTURE_ID"]);
	return this->getPictureById(picId);
}

std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User& user)
{
	//get picture id
	std::string getTags = "SELECT PICTURE_ID FROM TAGS WHERE USER_ID = " + std::to_string(user.getId()) + ';';
	selected pictureIds;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getTags.c_str(), callback, &pictureIds, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get tags");
	}
	else if (pictureIds.empty())
	{
		throw std::exception("user weren't tagged");
	}
	std::list<Picture> picturesToReturn;
	for (auto& picId : pictureIds)
	{
		picturesToReturn.push_back(this->getPictureById(std::get<int>(picId["PICTURE_ID"])));
	}
	return picturesToReturn;
}

bool DatabaseAccess::open()
{
	bool doesFileExists = _access(DB_NAME, 0) == FILE_EXISTS;
	int res = sqlite3_open(DB_NAME, &this->_db);
	if (!doesFileExists && res == SQLITE_OK)
	{
		char* err = 0;
		if (sqlite3_exec(this->_db, ENABLE_FOREIGN_KEY, nullptr, nullptr, &err) != SQLITE_OK)
		{
			sqlite3_free(err);
			return false;
		}
		return this->defineDb();
	}
	return !res;
}

void DatabaseAccess::close()
{
	int res = sqlite3_close(_db);
	if (res != SQLITE_OK)
	{
		throw std::exception("can't close the dataBase!");
	}
}

void DatabaseAccess::clear()
{
}

bool DatabaseAccess::defineDb() const
{
	int res = 0;
	char* err = nullptr;
	//queries to define db
	const std::vector<std::string> defineQueries = {
	"CREATE TABLE USERS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL);",
	"CREATE TABLE ALBUMS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, CREATION_DATE TEXT NOT NULL, USER_ID INTEGER NOT NULL, FOREIGN KEY (USER_ID) REFERENCES USERS(ID) ON DELETE CASCADE);",
	"CREATE TABLE TAGS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, PICTURE_ID INTEGER NOT NULL, USER_ID INTEGER NOT NULL, FOREIGN KEY (USER_ID) REFERENCES USERS(ID) ON DELETE CASCADE, FOREIGN KEY (PICTURE_ID) REFERENCES PICTURES(ID) ON DELETE CASCADE);",
	"CREATE TABLE PICTURES (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CREATION_DATE TEXT NOT NULL, ALBUM_ID INTEGER NOT NULL, FOREIGN KEY (ALBUM_ID) REFERENCES ALBUMS(ID) ON DELETE CASCADE);"
	};
	this->startTransaction();

	for (const auto& query : defineQueries)
	{
		res = sqlite3_exec(this->_db, query.c_str(), nullptr, nullptr, &err);
		if (res != SQLITE_OK)
		{
			sqlite3_free(err);
			sqlite3_exec(this->_db, "ROLLBACK;", nullptr, nullptr, nullptr);
			return false;
		}
	}

	this->commitTransaction();
	return true;
}

void DatabaseAccess::startTransaction() const
{
	char* err = nullptr;
	//start a transaction
	if (sqlite3_exec(this->_db, "BEGIN TRANSACTION;", nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cannot start transaction");
	}
}

void DatabaseAccess::commitTransaction() const
{
	char* err;
	//commit a transaction
	if (sqlite3_exec(this->_db, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK)
	{
		throw std::exception("failed to commit changes");
	}
}

void DatabaseAccess::checkSqlInjection(std::string stringToCheck)
{
	if (stringToCheck.find("'") != std::string::npos || stringToCheck.find(";") != std::string::npos)
	{
		throw std::exception("SQL injection attempt!!");
	}
}

void DatabaseAccess::addTags(const Picture& pic, const std::string& pictureId) const
{
	char* err = nullptr;
	for (const auto& tag : pic.getUserTags())
	{
		if (!this->doesUserExists(tag))
		{
			continue;
		}
		//add new tags
		std::string insertTag = "INSERT INTO TAGS (PICTURE_ID, USER_ID) VALUES (" + pictureId + ", " + std::to_string(tag) + ");";
		if (sqlite3_exec(this->_db, insertTag.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			sqlite3_free(err);
			throw std::exception("cannot create tag");
		}
	}
}

Picture DatabaseAccess::getPictureById(const int picId)
{
	selected returnedData;
	char* err = nullptr;
	//get picture by its id
	std::string getPicture = "SELECT * FROM PICTURES WHERE ID = " + std::to_string(picId) + ";";
	if (sqlite3_exec(this->_db, getPicture.c_str(), callback, &returnedData, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get picture with id");
	}
	if (returnedData.empty())
	{
		throw std::exception("picture does not exist");
	}
	Picture picToReturn(picId, std::get<std::string>(returnedData[0]["NAME"]), std::get<std::string>(returnedData[0]["LOCATION"]), std::get<std::string>(returnedData[0]["CREATION_DATE"]));
	//get tags of picture
	std::string getTags = "SELECT USER_ID FROM TAGS WHERE PICTURE_ID = " + std::to_string(picId) + ';';
	returnedData.clear();
	if (sqlite3_exec(this->_db, getTags.c_str(), callback, &returnedData, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get tags");
	}
	else if (returnedData.empty())
	{
		return picToReturn;
	}
	for (auto& tag : returnedData)
	{
		picToReturn.tagUser(std::get<int>(tag["USER_ID"]));
	}
	return picToReturn;
}

void DatabaseAccess::removeTags(const int pictureId)
{
	//delete tags
	std::string deleteTagsQ = "DELETE FROM TAGS WHERE PICTURE_ID = " + std::to_string(pictureId) + ";";
	char* err = nullptr;
	if (sqlite3_exec(this->_db, deleteTagsQ.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant delete tags");
	}
}

int DatabaseAccess::callback(void* data, int argc, char** argv, char** azColName)
{
	//casting the data
	selected* rows = (selected*)data;
	rows->emplace_back();
	auto& row = rows->back();
	for (int i = 0; i < argc; i++)
	{
		std::string value(argv[i]);
		if (std::all_of(value.begin(), value.end(), std::isdigit))
		{
			row[azColName[i]] = std::stoi(value);
		}
		else
		{
			row[azColName[i]] = value;
		}
	}
	return 0;
}

const std::list<Album> DatabaseAccess::getAlbums()
{
	//get all albums
	std::string getAlbumsQ = "SELECT * FROM ALBUMS;";
	selected returnedAlbums;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getAlbumsQ.c_str(), callback, &returnedAlbums, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get albums");
	}
	if (returnedAlbums.empty())
	{
		throw std::exception("no albums");
	}
	std::list<Album> albumsToReturn;
	for (auto& album : returnedAlbums)
	{
		Album albumToReturn(std::get<int>(album["USER_ID"]), std::get<std::string>(album["NAME"]), std::get<std::string>(album["CREATION_DATE"]));
		//get pictures of album
		std::string getPictures = "SELECT ID FROM PICTURES WHERE ALBUM_ID = " + std::to_string(std::get<int>(album["ID"])) + ";";
		selected picturesID;
		if (sqlite3_exec(this->_db, getPictures.c_str(), callback, &picturesID, &err) != SQLITE_OK)
		{
			sqlite3_free(err);
			throw std::exception("cant get pictures");
		}
		if (!picturesID.empty())
		{
			for (auto& id : picturesID)
			{
				albumToReturn.addPicture(this->getPictureById(std::get<int>(id["ID"])));
			}
		}
		albumsToReturn.push_back(albumToReturn);
	}
	return albumsToReturn;
}

const std::list<Album> DatabaseAccess::getAlbumsOfUser(const User& user)
{
	//get albums of user by id
	std::string getAlbumsQ = "SELECT * FROM ALBUMS WHERE USER_ID = " + std::to_string(user.getId()) + ";";
	selected returnedAlbums;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getAlbumsQ.c_str(), callback, &returnedAlbums, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get albums");
	}
	if (returnedAlbums.empty())
	{
		throw std::exception("user have no albums");
	}
	std::list<Album> albumsToReturn;
	for (auto& album : returnedAlbums)
	{
		albumsToReturn.push_back(Album(std::get<int>(album["USER_ID"]), std::get<std::string>(album["NAME"]), std::get<std::string>(album["CREATION_DATE"])));
	}
	return albumsToReturn;
}

void DatabaseAccess::createAlbum(const Album& album)
{
	this->checkSqlInjection(album.getName());
	char* err = nullptr;
	if (this->doesAlbumExists(album.getName(), album.getOwnerId()))
	{
		throw std::exception("album with this name already exist");
	}
	//add new album
	std::string insertAlbum = "INSERT INTO ALBUMS (NAME, CREATION_DATE, USER_ID) VALUES ('" + album.getName() + "', '" + album.getCreationDate() + "', " + std::to_string(album.getOwnerId()) + ");";
	try
	{
		this->startTransaction();
		if (sqlite3_exec(this->_db, insertAlbum.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			throw std::exception("cannot create album");
		}
		std::string albumId = std::to_string(sqlite3_last_insert_rowid(this->_db));
		for (const auto& picture : album.getPictures())
		{
			//create picture
			this->addPictureToAlbumByName(album.getName(), picture, album.getOwnerId());
		}
		this->commitTransaction();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		sqlite3_free(err);
		sqlite3_exec(this->_db, "ROLLBACK;", nullptr, nullptr, nullptr);
	}
}

void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
	this->checkSqlInjection(albumName);
	if (this->doesAlbumExists(albumName, userId) == false)
	{
		throw std::exception("album does not exist");
	}
	char* err = nullptr;
	this->startTransaction();
	try
	{
		//delete album
		std::string deleteAlbum = "DELETE FROM ALBUMS WHERE USER_ID = " + std::to_string(userId) + " AND NAME = '" + albumName + "';";
		if (sqlite3_exec(this->_db, deleteAlbum.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			throw std::exception("failed to delete album");
		}
		this->commitTransaction();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		sqlite3_free(err);
		sqlite3_exec(this->_db, "ROLLBACK;", nullptr, nullptr, nullptr);
	}
}

bool DatabaseAccess::doesAlbumExists(const std::string& albumName, int userId) const
{
	this->checkSqlInjection(albumName);
	//get album id
	std::string checkQuery = "SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID = " + std::to_string(userId) + ';';
	selected id;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, checkQuery.c_str(), callback, &id, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("error getting album");
	}
	if (id.empty())
	{
		return false;
	}
	return true;
}

Album DatabaseAccess::openAlbum(const std::string& albumName, const int userId)
{
	//get album by name
	std::string getAlbum = "SELECT * FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID = " + std::to_string(userId) + ";";
	selected album;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getAlbum.c_str(), callback, &album, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get album");
	}
	else if (album.empty())
	{
		throw std::exception("album does not exist");
	}
	Album albumToReturn(std::get<int>(album[0]["USER_ID"]), albumName, std::get<std::string>(album[0]["CREATION_DATE"]));

	std::string getPicturesId = "SELECT ID FROM PICTURES WHERE ALBUM_ID = " + std::to_string(std::get<int>(album[0]["ID"])) + ';';
	selected pictureId;
	if (sqlite3_exec(this->_db, getPicturesId.c_str(), callback, &pictureId, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get pictures");
	}
	else if (pictureId.empty())
	{
		return albumToReturn;
	}
	for (auto& id : pictureId)
	{
		albumToReturn.addPicture(this->getPictureById(std::get<int>(id["ID"])));
	}
	return albumToReturn;
}

void DatabaseAccess::closeAlbum(Album& pAlbum)
{
	
}

void DatabaseAccess::printAlbums()
{
	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	std::list<Album> albums = this->getAlbums();
	if (albums.empty())
	{
		std::cout << "there is not albums" << std::endl;
	}
	for (const auto& album : albums)
	{
		std::cout << std::setw(5) << "* " << album;
	}
}

void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture, const int ownerId)
{
	this->checkSqlInjection(albumName);
	this->checkSqlInjection(picture.getName());
	//get id from albums by name
	std::string getAlbumId = "SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID = " + std::to_string(ownerId) + ";";
	selected albumId;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getAlbumId.c_str(), callback, &albumId, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant find album");
	}
	else if (albumId.empty())
	{
		throw std::exception("album does not exist");
	}
	try
	{
		this->startTransaction();
		//add picture
		std::string insertPicture = "INSERT INTO PICTURES (NAME, LOCATION, CREATION_DATE, ALBUM_ID) VALUES ('" + picture.getName() + "', '" + picture.getPath() + "', '" + picture.getCreationDate() + "', " + std::to_string(std::get<int>(albumId[0]["ID"])) + ");";
		if (sqlite3_exec(this->_db, insertPicture.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			sqlite3_free(err);
			throw std::exception("cant create picture");
		}
		std::string pictureId = std::to_string(sqlite3_last_insert_rowid(this->_db));
		this->addTags(picture, pictureId);
		this->commitTransaction();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		sqlite3_free(err);
		sqlite3_exec(this->_db, "ROLLBACK;", nullptr, nullptr, nullptr);
	}
}

void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName, const int ownerId)
{
	this->checkSqlInjection(albumName);
	this->checkSqlInjection(pictureName);
	//get album id by name
	std::string getAlbumId = "SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID = " + std::to_string(ownerId) + ";";
	selected ids; // first - albumID
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getAlbumId.c_str(), callback, &ids, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant find album");
	}
	else if (ids.empty())
	{
		throw std::exception("album does not exist");
	}
	try
	{
		this->startTransaction();
		//delete pictures
		std::string insertPicture = "DELETE FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = " + std::to_string(std::get<int>(ids[0]["ID"])) + ";";
		if (sqlite3_exec(this->_db, insertPicture.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
		{
			sqlite3_free(err);
			throw std::exception("cant delete picture");
		}
		this->commitTransaction();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		sqlite3_free(err);
		sqlite3_exec(this->_db, "ROLLBACK;", nullptr, nullptr, nullptr);
	}
}

void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	this->checkSqlInjection(albumName);
	this->checkSqlInjection(pictureName);
	if (!this->doesUserExists(userId))
	{
		throw std::exception("user does not exist");
	}

	//get picture id
	std::string getPictureId = "SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "';";
	selected pictureId;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getPictureId.c_str(), callback, &pictureId, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("failed to get picture id");
	}
	if (pictureId.empty())
	{
		throw std::exception("picture does not exist");
	}
	//create tag
	std::string insertTag = "INSERT INTO TAGS (PICTURE_ID, USER_ID)	VALUES (" + std::to_string(std::get<int>(pictureId[0]["ID"])) + ", " + std::to_string(userId) + ");";
	if (sqlite3_exec(this->_db, insertTag.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("failed to create tag");
	}
}

void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	this->checkSqlInjection(albumName);
	this->checkSqlInjection(pictureName);
	if (!this->doesUserExists(userId))
	{
		throw std::exception("user does not exist");
	}
	//get picture id
	std::string getPictureId = "SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "';";
	selected pictureId;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getPictureId.c_str(), callback, &pictureId, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("failed to get picture id");
	}
	if (pictureId.empty())
	{
		throw std::exception("picture does not exist");
	}
	//delete tags from picture by user id 
	std::string deleteTag = "DELETE FROM TAGS WHERE PICTURE_ID = " + std::to_string(std::get<int>(pictureId[0]["ID"])) + " AND USER_ID = " + std::to_string(userId) + ";";
	if (sqlite3_exec(this->_db, deleteTag.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cannot delete tag");
	}
}

void DatabaseAccess::printUsers()
{
	selected users;
	char* err = nullptr;
	//get all users
	std::string getUsers = "SELECT * FROM USERS;";
	if (sqlite3_exec(this->_db, getUsers.c_str(), callback, &users, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get users");
	}
	if (users.empty())
	{
		throw std::exception("no users");
	}
	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (auto& user : users)
	{
		std::cout << std::setw(5) << "   + @" << std::get<int>(user["ID"]) << " - " << std::get<std::string>(user["NAME"]) << std::endl;
	}
}

int DatabaseAccess::createUser(User& user)
{
	this->checkSqlInjection(user.getName());

	//create user
	std::string insertUser = "INSERT INTO USERS (NAME) VALUES ('" + user.getName() + "');";
	char* err = nullptr;
	if (sqlite3_exec(this->_db, insertUser.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("failed to create user");
	}
	return sqlite3_last_insert_rowid(this->_db);
}

void DatabaseAccess::deleteUser(const User& user)
{
	if (!this->doesUserExists(user.getId()))
	{
		throw std::exception("user does not exist");
	}
	char* err = nullptr;
	//delete user
	std::string deleteUser = "DELETE FROM USERS WHERE ID = " + std::to_string(user.getId()) + ";";
	if (sqlite3_exec(this->_db, deleteUser.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cannot delete user");
	}
}

bool DatabaseAccess::doesUserExists(int userId) const
{
	//get users by id
	std::string getUser = "SELECT ID FROM USERS WHERE ID = " + std::to_string(userId) + ';';
	selected user;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getUser.c_str(), callback, &user, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get user");
	}
	else if (user.empty())
	{
		return false;
	}
	return true;
}

User DatabaseAccess::getUser(int userId)
{
	//get user by id
	std::string getUser = "SELECT NAME FROM USERS WHERE ID = " + std::to_string(userId) + ';';
	selected user;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getUser.c_str(), callback, &user, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get user");
	}
	else if (user.empty())
	{
		throw std::exception("user does not exist");
	}
	return User(userId, std::get<std::string>(user[0]["NAME"]));
}

int DatabaseAccess::countAlbumsOwnedOfUser(const User& user)
{
	//count albums by user id
	std::string getCount = "SELECT COUNT(*) FROM USERS WHERE ID = " + std::to_string(user.getId()) + ";";
	char* err = nullptr;
	selected count;
	if (sqlite3_exec(this->_db, getCount.c_str(), callback, &count, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("cant get count");
	}
	return std::get<int>(count[0]["COUNT(*)"]);
}

int DatabaseAccess::countAlbumsTaggedOfUser(const User& user)
{
	selected countPerPic;
	int count = 0;
	char* err = nullptr;
	for (const auto& album : this->getAlbums())
	{
		for (const auto& picture : album.getPictures())
		{
			if (picture.isUserTagged(user.getId()))
			{
				count++;
				break;
			}
		}
	}
	return count;
}

int DatabaseAccess::countTagsOfUser(const User& user)
{
	std::string countTags = "SELECT COUNT(*) FROM TAGS WHERE USER_ID = " + std::to_string(user.getId()) + ";";
	selected count;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, countTags.c_str(), callback, &count, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("error getting count");
	}
	else if (count.empty())
	{
		throw std::exception("error getting count");
	}
	return std::get<int>(count[0]["COUNT(*)"]);

}

float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
	float countTagsPerAlbum = this->countAlbumsTaggedOfUser(user);
	std::string getCountAlbums = "SELECT COUNT(*) FROM ALBUMS;";
	selected count;
	char* err = nullptr;
	if (sqlite3_exec(this->_db, getCountAlbums.c_str(), callback, &count, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		throw std::exception("error getting count of albums");
	}
	else if (count.empty())
	{
		throw std::exception("error getting count of albums"); // the have to be albums after the couuntAlbumsTaggedOfUser
	}
	return (std::get<int>(count[0]["COUNT(*)"]) == 0) ? 0.0 : countTagsPerAlbum / std::get<int>(count[0]["COUNT(*)"]);
}
