package rest.resources;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.PostConstruct;
import javax.inject.Singleton;
import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import rest.model.EnvDataRow;

@Singleton
@Path("environmentData")
public class EnvironmentDataResource {
	
	private List<EnvDataRow> envData;
	
	@PostConstruct
	private void init(){
		envData = new ArrayList<>();
	}
	
	@GET
	@Produces(MediaType.APPLICATION_JSON)
	public List<EnvDataRow> getEnvironmentData(){
		return envData;
	}
	
	@POST
	@Consumes(MediaType.APPLICATION_JSON)
	public Response addData(EnvDataRow envDataRow){
		envData.add(envDataRow);
		return Response.status(201).build();
	}
	

}
