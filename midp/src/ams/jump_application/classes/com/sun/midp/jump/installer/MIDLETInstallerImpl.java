/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */


package com.sun.midp.jump.installer;

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.module.installer.JUMPInstallerModule;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.ArrayList;
import java.util.Map;
import java.util.Properties;

import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.jump.MIDletApplication;
import com.sun.midp.jump.JumpInit;
import com.sun.midp.jump.midletsuite.MIDletSuiteStorageAccessor;

/**
 * <code>JUMPInstallerModule</code> provides the ability to install
 * content.
 * Installers have to implement this interface. They can optionally derive from
 * {@link com.sun.jump.module.contentstore.JUMPContentStore} in their
 * implementation for flexible storage options and abstractions.
 */
public class MIDLETInstallerImpl implements JUMPInstallerModule {
    
    JUMPInstallerInterface     installer = null;
    StorageAccessInterface     suiteStore = null;

    private static String midpHomeKey    = "sun.midp.home.path";

    public void unload() {
    }
    
    public void load(Map map) {

        String homeDir = (String) map.get(midpHomeKey);

        JumpInit.init(homeDir);

        installer = new JUMPFileInstaller();

        suiteStore = new MIDletSuiteStorageAccessor();
    }

    /**
     * Install content specified by the given descriptor and location.
     * @return the installed content
     */
    public JUMPContent[] install(URL location, JUMPDownloadDescriptor desc) {
	return installOrUpdate(location, desc, false);
    }
    
    /**
     * Update content from given location
     */
    public void update(JUMPContent content, URL location,
		       JUMPDownloadDescriptor desc) {
	installOrUpdate(location, desc, true);
    }
    
    private JUMPContent[] installOrUpdate(URL location, 
					  JUMPDownloadDescriptor desc, 
					  boolean isUpdate) {
	
	String path = location.getPath().toLowerCase();
	
	// below for more details.
	String bundleName = desc.getName();
	if (bundleName != null) {
	    // We need to replace spaces because apparently java doesn't like
	    // jarfiles with spaces in the name. Any further string
	    // substitutions should be done here.
	    bundleName = bundleName.replace(' ', '_');
	}
	
	
	String localJadFile = null;
	String localJarFile = null;
	
	try {
	    
	    Properties prop = desc.getApplications()[0];
	    localJadFile = prop.getProperty("JUMPApplication_localJadUrl");
	    localJarFile = location.getPath();
	    
	    int suiteId = 0;
	    
	    if (localJadFile != null) {
		suiteId = installer.verifyAndStoreSuite(desc.getObjectURI(),
							null,
							localJadFile,
							localJarFile, isUpdate);
                
	    } else if (path.endsWith(".jar")) {
		suiteId = installer
		    .verifyAndStoreSuite(desc.getObjectURI(), 
					 localJarFile, bundleName, isUpdate);
	    } else {
		System.err.println("install() failed, path not a jar file: "
				   + location);
		return null;
	    }
	    
	    // Install succeeded. Gather the installed midlet suite's info 
	  // from suitestore and convert them to a list of JUMPContents.
	    
	    JUMPContent[] installed = suiteStore
		.convertToMIDletApplications(suiteId);
	    
	    return installed;
	    
	} catch (Throwable ex) {
	    handleInstallerException(ex);   
	} finally {
	    //           File localJad = new File(localJadFile);
	    //           if (localJad.exists()) {
	    //               localJad.delete();
	    //           }
	    //           File localJar = new File(localJarFile);
	    //           if (localJar.exists()) {
	    //               localJar.delete();
	    //           }           
	}
	
	return null;
	
    }
    
    /**
     * Uninstall content
     */
    public void uninstall(JUMPContent content) {
        MIDletApplication midlet = (MIDletApplication) content;
        
        JUMPContent midlets[] = suiteStore
	    .convertToMIDletApplications(midlet.getMIDletSuiteID());
        if (midlets.length > 1) {
	    System.out.println("MIDLET suite: " + midlet.getTitle()
				+ " contains the following midlets.");
	    System.out.print("  ");
	    for (int i = 0; i < midlets.length; i++) {
		JUMPApplication app = (JUMPApplication)midlets[i];
		System.out.print(app.getTitle());
		if (i < (midlets.length - 1)) {
		    System.out.print(", ");
		}
	    }
	    System.out.println("");
	    System.out.println("Deleting this suite will remove all" 
			       + " of the midlets.");
            
	    // while ( true ) {
	    //    System.out.println("Do you wish to proceed: [y/n]");
	    //    BufferedReader in =
	    //       new BufferedReader( new InputStreamReader( System.in ) );
	    //    String answer;
	    //                
	    //    try {
	    //         answer = in.readLine();
	    //    } catch ( java.io.IOException ioe ) {
	    //         continue;
	    //    }
	    //                
	    //  if (answer.toLowerCase().equals("y")) {
	    //       break;
	    //  } else if (answer.toLowerCase().equals("n")){
	    //       return;
	    //  } else {
	    //       System.out.println("ERROR: Illegal response.");
	    //    }
	    //  }
        }
        suiteStore.remove(midlet.getMIDletSuiteID());
    }
    
    /**
     * Get all installed content
     */
    public JUMPContent[] getInstalled() {
	
	ArrayList appslist = new ArrayList();
	JUMPContent[] apps;
	
	int[] suiteIds = suiteStore.getInstalledMIDletSuiteIds();
	
	for (int i = 0; i < suiteIds.length; i++) {
            apps = suiteStore.convertToMIDletApplications(suiteIds[i]);
            for (int j = 0; j < apps.length; j++) {
                appslist.add(apps[j]);
            }
	}
	
	return (JUMPContent[])appslist.toArray(new JUMPContent[] {});
    }
    
    /**
     * Converts a pair suite id and midlet class name into
     * <code>MIDletApplication</code>.
     *
     * @param midletSuiteId <code>MIDlet</code> suite ID
     * @param midletClassName name of <code>MIDlet</code> class
     *
     * @return instance of <code>MIDletApplication</code> or <code>null</code>
     *  if there is no matching instance
     */
    public MIDletApplication getMIDletApplication(
            final int midletSuiteId, final String midletClassName) {
        final MIDletApplication [] apps = (MIDletApplication [])
            suiteStore.convertToMIDletApplications(midletSuiteId);
        for (int i = 0; i < apps.length; i++) {
            final MIDletApplication app = apps[i];
            if (app.getMIDletClassName().equals(midletClassName)) {
                return app;
            }
        }
        return null;
    }
   
    /**
     * Handles an installer exceptions.
     *
     * @param ex exception to handle
     */
    static void handleInstallerException(Throwable ex) {

        String message = null;

        // ex.printStackTrace();

        if (ex instanceof InvalidJadException) {
            InvalidJadException ije = (InvalidJadException)ex;
            int reason = ije.getReason();

            message = "** Error installing suite (" + reason + "): " +
                messageForInvalidJadException(ije);
        } else if (ex instanceof IOException) {
            message = "** I/O Error installing suite: " + ex.getMessage();
        } else {
            message = "** Error installing suite: " + ex.toString();
        }

        if (message != null) {
            System.err.println(message);
        }
    }

    /**
     * Returns the associated message for the given exception.
     * This function is here instead of in the exception its self because
     * it not need on devices, it needed only on development platforms that
     * have command line interface.
     *
     * @param ije reason reason code for the exception
     *
     * @return associated message for the given reason
     */
    static String messageForInvalidJadException(InvalidJadException ije) {
        switch (ije.getReason()) {
        case InvalidJadException.MISSING_PROVIDER_CERT:
        case InvalidJadException.MISSING_SUITE_NAME:
        case InvalidJadException.MISSING_VENDOR:
        case InvalidJadException.MISSING_VERSION:
        case InvalidJadException.MISSING_JAR_URL:
        case InvalidJadException.MISSING_JAR_SIZE:
        case InvalidJadException.MISSING_CONFIGURATION:
        case InvalidJadException.MISSING_PROFILE:
            return "A required attribute is missing";

        case InvalidJadException.SUITE_NAME_MISMATCH:
        case InvalidJadException.VERSION_MISMATCH:
        case InvalidJadException.VENDOR_MISMATCH:
            return "A required suite ID attribute in the JAR manifest " +
                "do not match the one in the JAD";

        case InvalidJadException.ATTRIBUTE_MISMATCH:
            return "The value for " + ije.getExtraData() + " in the " +
                "trusted JAR manifest did not match the one in the JAD";

        case InvalidJadException.CORRUPT_PROVIDER_CERT:
            return "The content provider certificate cannot be decoded.";

        case InvalidJadException.UNKNOWN_CA:
            return "The content provider certificate issuer " +
                ije.getExtraData() + " is unknown.";

        case InvalidJadException.INVALID_PROVIDER_CERT:
            return "The signature of the content provider certificate " +
                "is invalid.";

        case InvalidJadException.CORRUPT_SIGNATURE:
            return "The JAR signature cannot be decoded.";

        case InvalidJadException.INVALID_SIGNATURE:
            return "The signature of the JAR is invalid.";

        case InvalidJadException.UNSUPPORTED_CERT:
            return "The content provider certificate is not a supported " +
                "version.";

        case InvalidJadException.EXPIRED_PROVIDER_CERT:
            return "The content provider certificate is expired.";

        case InvalidJadException.EXPIRED_CA_KEY:
            return "The public key of " + ije.getExtraData() +
                " has expired.";

        case InvalidJadException.JAR_SIZE_MISMATCH:
            return "The Jar downloaded was not the size in the JAD";

        case InvalidJadException.OLD_VERSION:
            return "The application is an older version of one that is " +
                "already installed";

        case InvalidJadException.NEW_VERSION:
            return "The application is an newer version of one that is " +
                "already installed";

        case InvalidJadException.INVALID_JAD_URL:
            return "The JAD URL is invalid";

        case InvalidJadException.JAD_SERVER_NOT_FOUND:
            return "JAD server not found";

        case InvalidJadException.JAD_NOT_FOUND:
            return "JAD not found";

        case InvalidJadException.INVALID_JAR_URL:
            return "The JAR URL in the JAD is invalid: " +
                ije.getExtraData();

        case InvalidJadException.JAR_SERVER_NOT_FOUND:
            return "JAR server not found: " + ije.getExtraData();

        case InvalidJadException.JAR_NOT_FOUND:
            return "JAR not found: " + ije.getExtraData();

        case InvalidJadException.CORRUPT_JAR:
            return "Corrupt JAR, error while reading: " +
                ije.getExtraData();

        case InvalidJadException.INVALID_JAR_TYPE:
            if (ije.getExtraData() != null) {
                return "JAR did not have the correct media type, it had " +
                    ije.getExtraData();
            }

            return "The server did not have a resource with an " +
                "acceptable media type for the JAR URL. (code 406)";

        case InvalidJadException.INVALID_JAD_TYPE:
            if (ije.getExtraData() != null) {
                String temp = ije.getExtraData();

                if (temp.length() == 0) {
                    return "JAD did not have a media type";
                }

                return "JAD did not have the correct media type, it had " +
                    temp;
            }

            /*
             * Should not happen, the accept field is not send
             * when getting the JAD.
             */
            return "The server did not have a resource with an " +
                "acceptable media type for the JAD URL. (code 406)";

        case InvalidJadException.INVALID_KEY:
            return "The attribute key [" + ije.getExtraData() +
                "] is not in the proper format";

        case InvalidJadException.INVALID_VALUE:
            return "The value for attribute " + ije.getExtraData() +
                " is not in the proper format";

        case InvalidJadException.INSUFFICIENT_STORAGE:
            return "There is insufficient storage to install this suite";

        case InvalidJadException.UNAUTHORIZED:
            return "Authentication required or failed";

        case InvalidJadException.JAD_MOVED:
            return "The JAD to be installed is for an existing suite, " +
                "but not from the same domain as the existing one: " +
                ije.getExtraData();

        case InvalidJadException.CANNOT_AUTH:
            return
                "Cannot authenticate with the server, unsupported scheme";

        case InvalidJadException.DEVICE_INCOMPATIBLE:
            return "Either the configuration or profile is not supported.";

        case InvalidJadException.ALREADY_INSTALLED:
            return
                "The JAD matches a version of a suite already installed.";

        case InvalidJadException.AUTHORIZATION_FAILURE:
            return "The suite is not authorized for " + ije.getExtraData();

        case InvalidJadException.PUSH_DUP_FAILURE:
            return "The suite is in conflict with another application " +
                "listening for network data on " + ije.getExtraData();

        case InvalidJadException.PUSH_FORMAT_FAILURE:
            return "Push attribute in incorrectly formated: " +
                ije.getExtraData();

        case InvalidJadException.PUSH_PROTO_FAILURE:
            return "Connection in push attribute is not supported: " +
                ije.getExtraData();

        case InvalidJadException.PUSH_CLASS_FAILURE:
            return "The class in push attribute not in a MIDlet-<n> " +
                "attribute: " + ije.getExtraData();

        case InvalidJadException.TRUSTED_OVERWRITE_FAILURE:
            return "Cannot update a trusted suite with an untrusted " +
                "version";

        case InvalidJadException.INVALID_CONTENT_HANDLER:
	    return "Content handler attribute(s) incorrectly formatted: " +
		ije.getExtraData();

	case InvalidJadException.CONTENT_HANDLER_CONFLICT:
	    return "Content handler would conflict with another handler: " +
		ije.getExtraData();

        case InvalidJadException.CA_DISABLED:
            return "The application can't be authorized because " +
                ije.getExtraData() + " is disabled.";

        case InvalidJadException.UNSUPPORTED_CHAR_ENCODING:
            return "Unsupported character encoding: " + ije.getExtraData();
        }

        return ije.getMessage();
    }
}
