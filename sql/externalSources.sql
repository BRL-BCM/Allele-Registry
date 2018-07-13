
GRANT ALL ON `externalSources`.* TO 'externalSources'@'localhost' IDENTIFIED BY 'externalSources';
FLUSH PRIVILEGES;


CREATE DATABASE `externalSources` DEFAULT CHARACTER SET latin1 DEFAULT COLLATE latin1_general_cs;

USE `externalSources`;


CREATE TABLE sources (
  srcId      TINYINT UNSIGNED NOT NULL,
  srcName    VARCHAR(63)      NOT NULL,
  url        VARCHAR(511)     NOT NULL,
  paramTypes ENUM('','I','S','II','IS','SI','SS','III','IIS','ISI','ISS','SII','SIS','SSI','SSS') NOT NULL,
  guiSrcName VARCHAR(127)     NOT NULL,
  guiLabel   VARCHAR(63)      NOT NULL,   
  guiUrl     VARCHAR(511)     NOT NULL,
  PRIMARY KEY (srcId),
  UNIQUE (srcName),
  CHECK (srcId > 0 AND srcId <= 250)
);


CREATE TABLE links (
  extId  INTEGER UNSIGNED NOT NULL,
  srcId  TINYINT UNSIGNED NOT NULL,
  params VARBINARY(384)   NOT NULL,
  PRIMARY KEY (extId,srcId,params),
  INDEX (srcId,params,extId)
);


CREATE TABLE users (
  srcId     TINYINT UNSIGNED NOT NULL,
  userName  VARCHAR(255)     NOT NULL,
  PRIMARY KEY (srcId, userName)
);


ALTER TABLE links ADD CONSTRAINT FK_links_sources FOREIGN KEY (srcId) REFERENCES sources(srcId);
ALTER TABLE users ADD CONSTRAINT FK_users_sources FOREIGN KEY (srcId) REFERENCES sources(srcId);
