package rest.main;

import java.net.URI;
import java.net.URISyntaxException;

import javax.swing.JOptionPane;

import org.glassfish.jersey.jdkhttp.JdkHttpServerFactory;
import org.glassfish.jersey.server.ResourceConfig;

import com.sun.net.httpserver.HttpServer;

import rest.resources.EnvironmentDataResource;

public class StartServer {

	public static void main(String[] args) {
		try {
			HttpServer server = JdkHttpServerFactory.createHttpServer(new URI("http://localhost:5000/rest"),
					new ResourceConfig(EnvironmentDataResource.class));
			JOptionPane.showMessageDialog(null, "Stop");
			server.stop(0);
		} catch (URISyntaxException e) {
			e.printStackTrace();
		}

	}

}
